
#include "mrloop.h"

#define BUFSIZE 64*1024

typedef struct _conn
{
  int fd;
  char buf[BUFSIZE];
  struct iovec iov;
} conn_t;

static mrLoop *loop = NULL;

void *setup_conn(int fd, char **buf, int *buflen ) {
  conn_t *conn = calloc( 1, sizeof(conn_t) );
  conn->fd = fd;
  *buf = conn->buf;
  *buflen = BUFSIZE;
  return conn;
}

void on_data(void *conn, int fd, ssize_t nread, char *buf) {
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

  loop = createLoop(sig_handler);
  mrTcpServer( loop, 12345, setup_conn, on_data );
  runLoop(loop);
  freeLoop(loop);

}
