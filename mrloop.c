
#include "mrloop.h"
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define DBG if(0)

static int mrfd;
static char tmp[32*1024];
static int tmplen = 32*1024;
static int num_sqes = 0;

/*
static void print_buffer( char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
    //printf( "%c",b[z]);
  }
  printf("\n");
}

static void sig_handler(const int sig) {
  printf("mrloop Signal handled: %s.\n", strsignal(sig));
  close(mrfd);
  exit(EXIT_SUCCESS);
}
*/
mrLoop *createLoop(sigCB *sig) {

  signal(SIGINT, sig);
  signal(SIGTERM, sig);

  mrLoop *loop = calloc( 1, sizeof(mrLoop) );
  loop->ring = calloc( 1, sizeof(struct io_uring) );
  if ( io_uring_queue_init(128, loop->ring, 0) ) {
    perror("io_uring_setup");
    printf("Loop creation failed\n");
    return NULL;
  }

  loop->writeDataEvent = calloc( 1, sizeof(event_t) );
  loop->writeDataEvent->type = WRITE_DATA_EV;
  for (int i = 0; i < MAX_CONN; i++) {
    loop->readEvents[i] = calloc( 1, sizeof(event_t) );
    loop->readDataEvents[i] = calloc( 1, sizeof(event_t) );
    loop->readEvents[i]->type = READ_EV;
    loop->readDataEvents[i]->type = READ_DATA_EV;
  }

  return loop;
}

int setnonblocking(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFD,0) | O_NONBLOCK) == -1) {
        printf("set blocking error: %d\n", errno);
        return -1;
    }
    return 0;
}

void freeLoop(mrLoop *loop) {
  free(loop->ring);
  free(loop); 
}

void stopLoop(mrLoop *loop) {
  loop->stop = 1;
}

void _urpoll( mrLoop *loop, int fd, event_t *ev ) {

  DBG printf(" urpoll fd %d type %d\n",fd, ev->type);
  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  io_uring_prep_poll_add( sqe, fd, POLLIN );
  sqe->user_data = (unsigned long)ev;
  io_uring_submit(loop->ring);

}

void _addTimer( mrLoop *loop, event_t *ev ) {

  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  if (!sqe) { printf("child: get sqe failed\n"); return; }

  io_uring_prep_poll_add( sqe, ev->fd, POLLIN );
  io_uring_sqe_set_data(sqe, ev);
  io_uring_submit(loop->ring);
}


void _read( mrLoop *loop, event_t *ev ) {
  DBG printf(" _read fd %d\n", ev->fd);
  event_t *rdev = loop->readDataEvents[ev->fd];

  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  io_uring_prep_readv(sqe, rdev->fd, &(rdev->iov), 1, 0);
  sqe->user_data = (unsigned long)rdev;

  io_uring_submit(loop->ring); 

}

void _accept( mrLoop *loop, event_t *ev ) {


  struct sockaddr_in addr;
  socklen_t len;
  int cfd = accept(ev->fd, (struct sockaddr*)&addr, &len);


  if (cfd != -1) {
  //while ((cfd = accept(ev->fd, (struct sockaddr*)&addr, &len)) != -1) {
    //if (cfd >= MAX_CONN) { close(cfd); break; }

    DBG printf(" accept fd %d\n", cfd);
    mrfd = cfd;
    char *buf;
    int buflen;
    ev->user_data = ev->acb( cfd, &buf, &buflen);

    // Setup the read event
    event_t *rev  = loop->readEvents[cfd];
    event_t *rdev = loop->readDataEvents[cfd];
    rev->fd = cfd;
    //rev->user_data = ev->user_data;
    rdev->fd = cfd;
    rdev->rcb = ev->rcb;
    rdev->user_data = ev->user_data;
    rdev->iov.iov_base = buf;
    rdev->iov.iov_len = buflen;
    _urpoll( loop, cfd, rev );

    //readCompleteEvents[cfd].fd = cfd;
    //readCompleteEvents[cfd].cb = on_read_complete;

  } else {
    printf( " mrloop: _accept - error: %s\n", strerror(errno) );
    exit(-1);
  }
  // Keep listening
  _urpoll( loop, ev->fd, ev );

}

void runLoop( mrLoop *loop ) {
  struct io_uring_cqe *cqe;

  while ( 1 ) {
    DBG printf("before cqe\n");
    //TODO ret is?  int ret = io_uring_wait_cqe(loop->ring, &cqe);
    io_uring_wait_cqe(loop->ring, &cqe);
    event_t *ev = (event_t*)cqe->user_data;
    DBG printf(" ev? %p\n",ev);
    if ( !ev ) {
      io_uring_cqe_seen(loop->ring, cqe);
      continue; 
    }
    DBG printf(" ev type %d\n",ev->type);
    if ( ev->type == TIMER_EV ) {
      uint64_t value;
      int rd = read(ev->fd, &value, 8);
      //read(ev->fd, &value, 8);
      //printf("read %d from timerfd\n",rd);
      ev->tcb();
      _addTimer(loop, ev);
    }
    if ( ev->type == LISTEN_EV ) {
      _accept( loop, ev );
    }
    if ( ev->type == READ_EV ) {
      DBG printf(" read ev \n");
      _read(loop, ev);
    }
    if ( ev->type == READ_DATA_EV ) {

      if ( cqe->res == 0 ) {
        close(ev->fd);
      } else {
        DBG printf(" data ev res %d\n", cqe->res );
        DBG printf(" data ev fd %d\n", ev->fd );
        ev->rcb( ev->user_data, ev->fd, cqe->res, ev->iov.iov_base );
        //if ( cqe->res ) _urpoll( loop, ev->fd, loop->readEvents[ev->fd] );
        _urpoll( loop, ev->fd, loop->readEvents[ev->fd] );
        //io_uring_submit(loop->ring);  // poll submits
        num_sqes = 0;
        //printf("DELME after on_data num sqes now zero\n");
      }
    }

    //if ( ev2 ) ev2->cb(ev2->fd, cqe->res);
    io_uring_cqe_seen(loop->ring, cqe);
  }
}

int addTimer( mrLoop *loop, int seconds, timerCB *cb ) {

  struct itimerspec exp = {
    .it_interval = { seconds, 0},
    .it_value = { seconds, 0 },
  };
  int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (tfd < 0) { perror("timerfd_create"); return -1; }
  if (timerfd_settime(tfd, 0, &exp, NULL)) { perror("timerfd_settime"); close(tfd); return -1; }
  
  event_t *ev = malloc( sizeof(event_t) );
  ev->type = TIMER_EV;
  ev->fd = tfd;
  ev->tcb = cb;

  _addTimer(loop, ev);
  return 0;
}

int mrTcpServer( mrLoop *loop, int port, acceptCB *cb, readCB *rcb) { //, char *buf, int buflen ) {
  int listen_fd;
  struct sockaddr_in servaddr;
  int flags = 1;
  //socklen_t len = sizeof(struct sockaddr_in);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      printf("socket error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  if (setnonblocking(listen_fd) == -1) {
      printf("set non blocking error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
  setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
  //if (error != 0) perror("setsockopt");


  if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) == -1) {
      printf("bind error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  if (listen(listen_fd, 32) == -1) {
      printf("listen error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  event_t *ev = malloc( sizeof(event_t) );
  ev->type = LISTEN_EV;
  ev->fd = listen_fd;
  ev->acb = cb;
  ev->rcb = rcb;

  _urpoll( loop, listen_fd, ev );

  return 0;
}

int mrConnect( mrLoop *loop, char *addr, int port, readCB *rcb) {

  int fd, ret, on = 1;
  struct sockaddr_in sa;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Error creating socket: %s\n", strerror(errno));
    return -1;
  }

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  if (inet_aton(addr, &sa.sin_addr) == 0) {
    struct hostent *he;

    he = gethostbyname(addr);
    if (he == NULL) {
      //anetSetError(err, "can't resolve: %s\n", addr);
      close(fd);
      //return ANET_ERR;
      return -1;
    }
    memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
  }

  if (setnonblocking(fd) == -1) {
    printf("set non blocking error : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  ret = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
  if ( ret == -1 && errno != EINPROGRESS ) {
    printf("Error connecting socket: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  // Setup the read event
  event_t *rev  = loop->readEvents[fd];
  event_t *rdev = loop->readDataEvents[fd];
  rev->fd = fd;
  rev->user_data = 0;
  _urpoll( loop, fd, rev );

  rdev->fd = fd;
  rdev->rcb = rcb;
  rdev->user_data = 0;
  rdev->iov.iov_base = tmp;
  rdev->iov.iov_len = tmplen;

  return fd;
}

void mrFlush( mrLoop *loop ) {
  io_uring_submit(loop->ring); 
  num_sqes = 0;
}

void mrWritev( mrLoop *loop, int fd, struct iovec *iovs, int cnt ) {

  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  io_uring_prep_writev(sqe, fd, iovs, cnt, 0);
  sqe->user_data = 0;
  num_sqes += 1;
  if ( num_sqes > 64 ) { io_uring_submit(loop->ring); num_sqes = 0; }

}

void mrWritevf( mrLoop *loop, int fd, struct iovec *iovs, int cnt ) {

  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  io_uring_prep_writev(sqe, fd, iovs, cnt, 0);
  sqe->user_data = 0;
  io_uring_submit(loop->ring); 
  num_sqes = 0;

}

/*
void mrWritev2( mrLoop *loop, int fd, struct iovec *iovs, int cnt, void *user_data ) {

  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
  io_uring_prep_writev(sqe, fd, iovs, cnt, 0);
  sqe->user_data = user_data;
  num_sqes += 1;
  if ( num_sqes > 64 ) { io_uring_submit(loop->ring); num_sqes = 0; }

}



int io_uring_enter(unsigned int fd, unsigned int to_submit,
unsigned int min_complete, unsigned int flags,
sigset_t sig);

  ret = io_uring_enter(ring->ring_fd, 0, 4, IORING_ENTER_GETEVENTS, NULL);

void mrWrite( mrLoop *loop, int fd, char *buf, int buflen ) {
  DBG printf("mrWrite >%.*s<\n", buflen, buf);
  struct io_uring_sqe *sqe = io_uring_get_sqe(loop->ring);
 
  struct iovec *iov = &(loop->iovs[fd]); 
  iov->iov_base = buf;
  iov->iov_len = buflen;
  io_uring_prep_writev(sqe, fd, iov, 1, 0);
  sqe->user_data = 0;
  io_uring_submit(loop->ring); 
}

*/
