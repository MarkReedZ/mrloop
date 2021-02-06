//
// Write the file 'delme'
//

#include "mrloop.h"

static mr_loop_t *loop = NULL;

void on_write_done(void *user_data) {

    struct iovec *iov = (struct iovec*)user_data;
    printf("on_write_done - the file delme was written\n");
    free(iov->iov_base);
    free(iov);
    mr_stop(loop);
}

int main() {

    loop = mr_create_loop();

    struct iovec *iov = malloc(sizeof(struct iovec));
    iov->iov_base = malloc(1024);
    iov->iov_len  = 8;

    strcpy(iov->iov_base, "Testing  ");

    int fd = open("delme", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    mr_writevcb( loop, fd, iov, 1, iov, on_write_done );
    mr_flush(loop);

    mr_run(loop);
    mr_free(loop);

}

