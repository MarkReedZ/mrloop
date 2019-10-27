
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> 
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "liburing.h"
#include <sys/timerfd.h>
#include <sys/poll.h>

#define MAX_CONN 1024
#define BUFSIZE 4096
#define RX 0
#define TX 1

struct Connection {
    int fd;
    struct iovec iov[2];
    char buf[BUFSIZE];
};
struct Connection conns[MAX_CONN];

//static char buf[BUFSIZE];

static struct io_uring ring;
static struct io_uring_sqe *sqe;

int setnonblocking(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFD,0) | O_NONBLOCK) == -1) {
        printf("set blocking error: %d\n", errno);
        return -1;
    }
    return 0;
}

/* Types and data structures */
typedef void eventCB( int fd, int res );//, void *clientData);

/* File event structure */
typedef struct urEvent {
  eventCB *cb;
  void *clientData;
  int fd;
} urEvent;

static urEvent *acceptEvent;
static urEvent fdEvents[MAX_CONN];
static urEvent readCompleteEvents[MAX_CONN];
static urEvent writeCompleteEvents[MAX_CONN];


void on_timer(int fd, int res);

void timer( int seconds) {
  struct io_uring_sqe *sqe;
  sqe = io_uring_get_sqe(&ring);
  if (!sqe) {
    printf("child: get sqe failed\n");
    return;
  }

  struct itimerspec exp = {
    .it_interval = {},
    .it_value = { 2, 0 },
  };
  int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (tfd < 0) { perror("timerfd_create"); return; }
  if (timerfd_settime(tfd, 0, &exp, NULL)) { perror("timerfd_settime"); close(tfd); return; }

  urEvent *ev = malloc( sizeof(urEvent) );
  ev->fd = 5;
  ev->cb = on_timer;

  io_uring_prep_poll_add( sqe, tfd, POLLIN );
 	io_uring_sqe_set_data(sqe, ev);
  io_uring_submit(&ring);
}

void urpoll( int fd, urEvent *ev ) {

  sqe = io_uring_get_sqe(&ring);
  io_uring_prep_poll_add( sqe, fd, POLLIN );
 	sqe->user_data = (unsigned long)ev;
  io_uring_submit(&ring);

}

void on_timer(int fd, int res) { //, void *data) {
  //printf(" on_timer \n");
  timer(2);
}
void on_read(int fd, int res) { //, void *data) {
  //printf("  on_read \n");

  sqe = io_uring_get_sqe(&ring);
  io_uring_prep_readv(sqe, fd, &conns[fd].iov[RX], 1, 0);
 	sqe->user_data = (unsigned long)&readCompleteEvents[fd];
  io_uring_submit(&ring);

  //int nread = read(fd, buf, BUFSIZE);
  //if (nread <= 0) { close(fd); return; }
  //if (nread == 0) { close(fd); return; }
  //buf[nread] = 'a';
  //buf[nread + 1] = '\0';
  //write(fd, buf, nread);

  //urpoll( fd, &fdEvents[fd] );
  //sqe = io_uring_get_sqe(&ring);
  //io_uring_prep_poll_add( sqe, fd, POLLIN );
 	//sqe->user_data = (unsigned long)&fdEvents[fd];
  //io_uring_submit(&ring);

}
void on_read_complete(int fd, int res) { 
  //printf("  on_read_complete \n");

  if ( res == 0 ) { close(fd); return; }

  //printf( "DELME read complete data0 = %c \n", ((char*)(conns[fd].iov[TX].iov_base))[0]);
  //printf( "DELME read complete data1 = %c \n", ((char*)(conns[fd].iov[TX].iov_base))[1]);
  sqe = io_uring_get_sqe(&ring);
  conns[fd].iov[TX].iov_len = res;
  io_uring_prep_writev(sqe, fd, &conns[fd].iov[TX], 1, 0);
 	sqe->user_data = 0; //(unsigned long)&readCompleteEvents[fd];
  //io_uring_submit(&ring);

  sqe = io_uring_get_sqe(&ring);
  io_uring_prep_poll_add( sqe, fd, POLLIN );
 	sqe->user_data = (unsigned long)&fdEvents[fd];
  io_uring_submit(&ring);
}

void on_accept(int fd, int res) {
  //printf("  on_accept \n");

  struct sockaddr_in addr;
  socklen_t len;
  int cfd;

  while ((cfd = accept(fd, (struct sockaddr*)&addr, &len)) != -1) {
    if (cfd >= MAX_CONN) { close(cfd); break; }
    //printf( " accept fd %d\n", cfd );

    
    urEvent *ev = &fdEvents[cfd];
    ev->fd = cfd;
    ev->cb = on_read;
    urpoll( cfd, ev );
    readCompleteEvents[cfd].fd = cfd;
    readCompleteEvents[cfd].cb = on_read_complete;

  }

  // Keep listening
  urpoll( fd, acceptEvent );
}

/*
  sqe->poll_events = POLLIN;
  sqe->opcode = IORING_OP_POLL_ADD;
  sqe->flags = 0;
  sqe->ioprio = 0;
  sqe->fd = tfd;
  sqe->off = 0;
  sqe->addr = 0;
  sqe->len = 0;
  sqe->rw_flags = 0;
  sqe->user_data = 1;
  sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;
*/



int main() {

  for (size_t i = 0; i < MAX_CONN; ++i) {
    memset(&conns[i], 0, sizeof(conns[i]));
    conns[i].iov[RX].iov_base = conns[i].buf;
    conns[i].iov[RX].iov_len = BUFSIZE;
    conns[i].iov[TX].iov_base = conns[i].buf;
    conns[i].iov[TX].iov_len = 0;
  }

  int listen_fd;
  int conn_fd;
  int epoll_fd;
  int nread;
  int cur_fds;     // 当前已经存在的fd数量
  int wait_fds;    // epoll_wait的返回值
  int i;
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
  char buf[1024];
  socklen_t len = sizeof(struct sockaddr_in);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(12345);

  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      printf("socket error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  printf( " socket fd %d\n", listen_fd );

  // 4. 设置非阻塞模式
  if (setnonblocking(listen_fd) == -1) {
      printf("set non blocking error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  // 5. 绑定
  if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) == -1) {
      printf("bind error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  // 6. 监听
  if (listen(listen_fd, 32) == -1) {
      printf("listen error : %d ...\n", errno);
      exit(EXIT_FAILURE);
  }

  acceptEvent = malloc( sizeof(urEvent) );
  acceptEvent->fd = listen_fd;
  acceptEvent->cb = on_accept;

  struct io_uring_cqe *cqe;

  //int ret = io_uring_queue_init(128, &ring, IORING_SETUP_SQPOLL);
  int ret = io_uring_queue_init(128, &ring, 0);
  if (ret) {
    perror("io_uring_setup");
    printf("queue init fail ret=%d\n",ret);
    return 1;
  }

  //timer(2);

  urpoll( listen_fd, acceptEvent );

  while (1) {

    ret = io_uring_wait_cqe(&ring, &cqe);
    //printf("cqe res %d ud %lld\n",cqe->res, cqe->user_data);
    urEvent *ev2 = (urEvent*)cqe->user_data;
    if ( ev2 ) ev2->cb(ev2->fd, cqe->res);
    io_uring_cqe_seen(&ring, cqe);

  }
  

  //double start_time = clock();
  //double taken = ((double)(clock()-start_time))/CLOCKS_PER_SEC;
  //printf(" took %lf \n", taken);

  io_uring_queue_exit(&ring);

  return 0;
}
