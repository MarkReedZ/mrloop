// Read a file

#include "mrloop.h"

#define BUFSIZE 64*1024

static mr_loop_t *loop = NULL;
static int cnt = 0;

void on_read_done(void *user_data, int res) {

  struct iovec *iov = (struct iovec*)user_data;
  printf("on_read_done\n  Read: %.*s", 15, (char*)(iov->iov_base));
  free(iov->iov_base);
  free(iov);
  cnt += 1;
  if ( cnt == 2 ) exit(0);
}

int main() {

  loop = mr_create_loop();

  struct iovec *iov = malloc(sizeof(struct iovec));
  iov->iov_base = malloc(BUFSIZE);
  iov->iov_len  = BUFSIZE;

  int fd = open("read_file.c", O_RDONLY);

  mr_readvcb( loop, fd, iov, 1, 0, iov, on_read_done );

  iov = malloc(sizeof(struct iovec));
  iov->iov_base = malloc(BUFSIZE);
  iov->iov_len  = BUFSIZE;

  mr_readvcb( loop, fd, iov, 1, 1, iov, on_read_done );
  mr_flush(loop);

  mr_run(loop);
  mr_free(loop);

}

