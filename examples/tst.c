
#include "mrloop.h"

#define BUFSIZE 16*1024*1024

static char buf[BUFSIZE];

typedef struct _conn
{
  int fd;
  char buf[BUFSIZE];
} conn_t;

static mr_loop_t *loop = NULL;

void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}

static int cnt = 0;

void can_write( void *conn, int fd ) {
  int l = BUFSIZE;
  int nwritten = write(fd,buf,l);
  printf("can_write nwritten %d\n",nwritten);
  exit(-1);
}

void sendSomething(int fd) {

  int l = BUFSIZE;
  int nwritten = write(fd,buf,l);
  printf("send nwritten %d\n",nwritten);
  nwritten = write(fd,buf,l);
  printf("send2 nwritten %d\n",nwritten);
  mr_add_write_callback( loop, can_write, NULL, fd );  

}

void on_data(void *conn, int fd, ssize_t nread, char *buf) {
  //printf(" client on_data: %.*s\n", nread, buf );y
  //printf(" client on_data: %d\n", nread);
  //sendSomething(fd);
}

int main() {

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  loop = mr_create_loop(sig_handler);
  int fd = mr_connect(loop,"localhost", 12345, on_data);

  sendSomething(fd);

  mr_run(loop);
  mr_free(loop);

}

