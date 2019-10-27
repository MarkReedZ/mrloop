
#include "mrloop.h"

#define BUFSIZE 64*1024

typedef struct _conn
{
  int fd;
  char buf[BUFSIZE];
  struct iovec iov;
} conn_t;

static mrLoop *loop = NULL;

void on_timer() { 
  printf("tick\n");
}

void *setup_conn(int fd, char **buf, int *buflen ) {
  //printf("New Connection\n");
  conn_t *conn = calloc( 1, sizeof(conn_t) );
  conn->fd = fd;
  *buf = conn->buf;
  *buflen = BUFSIZE;
  return conn;
}

void on_data(void *conn, ssize_t nread, char *buf) {
  //printf("on_data fd %d >%.*s<\n", ((conn_t*)conn)->fd, nread, buf);
  //mrWrite( loop, ((conn_t*)conn)->fd, buf, nread );
  conn_t *c = conn;
  c->iov.iov_base = buf;
  c->iov.iov_len = nread;
  mrWritev( loop, ((conn_t*)conn)->fd, &(c->iov), 1 );

}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}


int main() {

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  
  loop = createLoop();
  mrTcpServer( loop, 12345, setup_conn, on_data );
  //addTimer(loop, 10, on_timer);
  runLoop(loop);
  freeLoop(loop);

}
