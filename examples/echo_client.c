
//
// Echo client that connects to port 12345
//

#include "mrloop.h"

#define BUFSIZE 64*1024

static char buf[BUFSIZE];
static mr_loop_t *loop = NULL;

void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}

static int cnt = 0;

void send_something(int fd) {

  char *p = buf;
  int l = 4;
  while ( l ) {
    int nwritten = write(fd,p,l);
    if (nwritten == 0) { printf("nwritten 0\n"); exit(-1); }
    if (nwritten == -1) { printf("nwritten -1\n"); printf("%s\n",strerror(errno)); exit(-1); }
    l -= nwritten;
    p += nwritten;
  } 

}

int on_data(void *conn, int fd, ssize_t nread, char *buf) {
  printf(" client on_data: %.*s\n", (int)nread, buf );
  send_something(fd);
}

int main() {

  signal(SIGINT,  sig_handler);
  signal(SIGTERM, sig_handler);

  loop = mr_create_loop(sig_handler);
  int fd = mr_connect(loop,"localhost", 12345, on_data);

  strcpy( buf, "test" );
  send_something(fd);

  mr_run(loop);
  mr_free(loop);

}

