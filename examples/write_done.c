
#include "mrloop.h"

#define BUFSIZE 16*1024

static mr_loop_t *loop = NULL;
static char buf[BUFSIZE];

typedef struct _conn
{
  int fd;
  char buf[BUFSIZE];
} conn_t;

void on_write_done(void *user_data) {
  printf("on_write_done - %ld\n", (unsigned long)user_data);
  exit(0);
}

void sendSomething(int fd) {

  struct iovec iov;
  iov.iov_base = buf;
  iov.iov_len = BUFSIZE;
  mr_writevcb( loop, fd, &iov, 1, (void*)1, on_write_done  );
  mr_flush(loop);
}

int on_data(void *conn, int fd, ssize_t nread, char *buf) { }

int main() {

  loop = mr_create_loop();
  int fd = mr_connect(loop,"localhost", 12345, on_data);

  sendSomething(fd);

  mr_run(loop);
  mr_free(loop);

}

