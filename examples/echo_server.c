
//
// Echo server that listens on port 12345
//

#include "mrloop.h"

#define BUFSIZE 64*1024

typedef struct _conn
{
  int fd;
  char buf[BUFSIZE];
  struct iovec iov;
} conn_t;

static mr_loop_t *loop = NULL;

void *setup_conn(int fd, char **buf, int *buflen ) {
  conn_t *conn = calloc( 1, sizeof(conn_t) );
  conn->fd = fd;
  *buf = conn->buf;
  *buflen = BUFSIZE;
  return conn;
}

int on_data(void *conn, int fd, ssize_t nread, char *buf) {
  conn_t *c = conn;
  
  // The client closed the connection
  if ( nread == 0 ) { 
    mr_close( loop, c->fd );
    free(c);
    return 1;
  }

  // Echo it back
  c->iov.iov_base = buf;
  c->iov.iov_len = nread;
  mr_writev( loop, ((conn_t*)conn)->fd, &(c->iov), 1 );
  return 0;
}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}

int main() {

  loop = mr_create_loop(sig_handler);
  mr_tcp_server( loop, 12345, setup_conn, on_data );
  mr_run(loop);
  mr_free(loop);

}
