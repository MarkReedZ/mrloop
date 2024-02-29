
// mr_writevcb example.  The echo_server needs to be running.  


#include "mrloop.h"

#define BUFSIZE 16*1024
static mr_loop_t *loop = NULL;
static char buf[BUFSIZE];

void on_write_done(void *user_data, int res) {

  printf("on_write_done - res %d user_data %ld\n", res, (unsigned long)user_data);
  if ( res < 0 ) {
    printf(" err: %s\n",strerror(-res));
  }
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

