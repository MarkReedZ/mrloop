
#include "mrloop.h"

static mr_loop_t *loop = NULL;
static int cnt = 0;

// Return 0 to stop the timer
int on_timer( void *user_data ) { 
  printf("tick\n");
  cnt += 1;
  if ( cnt > 5 ) {
    mr_stop(loop);
    return 0;
  }
  return 1;
}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}


int main() {

  loop = mr_create_loop(sig_handler);
  mr_add_timer(loop, 0.1, on_timer, NULL);
  mr_run(loop);
  mr_free(loop);

}
