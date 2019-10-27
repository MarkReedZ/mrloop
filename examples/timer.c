
#include "mrloop.h"

static mrLoop *loop = NULL;

void on_timer() { 
  printf("tick\n");
}

static void sig_handler(const int sig) {
  printf("Signal handled: %s.\n", strsignal(sig));
  exit(EXIT_SUCCESS);
}


int main() {

  loop = createLoop(sig_handler);
  addTimer(loop, 1, on_timer);
  runLoop(loop);
  freeLoop(loop);

}
