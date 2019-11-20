
#include "mrloop.h"

static mr_loop_t *loop = NULL;

void on_timer() { 
  printf("tick\n");
}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}


int main() {

  loop = mr_create_loop(sig_handler);
  mr_add_timer(loop, 0.1, on_timer);
  mr_run(loop);
  mr_free(loop);

}
