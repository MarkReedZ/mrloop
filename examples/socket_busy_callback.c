
#include "mrloop.h"

//
// Write a large amount of data to the socket so we get a partial write
// and need to wait until the socket is writable again
//
// Run the echo_server and then this.
//

#define BUFSIZE 16*1024*1024

static mr_loop_t *loop = NULL;
static char buf[BUFSIZE];

void can_write( void *conn, int fd ) {
  int l = BUFSIZE;
  int nwritten = write(fd,buf,l);
  printf("can_write nwritten %d\n",nwritten);
  exit(-1);
}

void sendSomething(int fd) {

  int l = BUFSIZE;
  int nwritten = write(fd,buf,l);
  printf("First nwritten %d\n",nwritten);
  nwritten = write(fd,buf,l);
  printf("Second nwritten %d\n",nwritten);
  mr_add_write_callback( loop, can_write, NULL, fd );  

}

void on_data(void *conn, int fd, ssize_t nread, char *buf) { }

int main() {

  loop = mr_create_loop();
  int fd = mr_connect(loop,"localhost", 12345, on_data);

  sendSomething(fd);

  mr_run(loop);
  mr_free(loop);

}

