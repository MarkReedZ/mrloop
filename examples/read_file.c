// Read a file

#include "mrloop.h"

#define BUFSIZE 64*1024

static mr_loop_t *loop = NULL;
static char buf[BUFSIZE];
static struct iovec *iov;

void on_read_done(void *user_data) {
  printf("on_read_done\n");
  printf("  Read: %.*s", 16, (char*)(iov->iov_base));
  free(iov->iov_base);
  free(iov);
  exit(0);
}

int main() {

  loop = mr_create_loop();

  iov = malloc(sizeof(struct iovec));
  iov->iov_base = malloc(BUFSIZE);
  iov->iov_len  = BUFSIZE;

  int fd = open("read_file.c", O_RDONLY);

  mr_readvcb( loop, fd, iov, 1, NULL, on_read_done );
  mr_flush(loop);

  mr_run(loop);
  mr_free(loop);

}

/*
  iov->iov_base = p;
  iov->iov_len = settings.block_size*1024*1024;

  if ( fsblock_min_block == -1 ) {
    fsblock_min_block = blk;
  }
  fsblock_size += 1;

  int fd = fsblock_fds[fsblock_index];
  mr_writevcb( settings.loop, fd, iov, 1, (void*)iov, blocks_on_write_done  );
  mr_flush(settings.loop);
*/
