
#include "mrloop.h"

static mr_loop_t *loop = NULL;

int cb( void *user_data ) { 
  static int n = 0;
  n += 1;
  printf("tick %d\n",n);
  if ( n >= 1100 ) {
    mr_stop(loop);
  } else {
    mr_call_soon(loop, cb, NULL);
  }
  return 0;
}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}

int main() {

  loop = mr_create_loop(sig_handler);

  for (int i = 0; i < 2; i++ ) {
    mr_call_soon(loop, cb, NULL);
  }
  mr_run(loop);
  mr_free(loop);

}
