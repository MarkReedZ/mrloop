
# MrLoop

Event loop for C using [io_uring](https://github.com/axboe/liburing) which requires linux kernel 5.4.1+

# Build

Install [liburing](https://github.com/axboe/liburing)

```
make
sudo make install
```

# Benchmarks

Echo server benchmarked with https://github.com/haraldh/rust_echo_bench

```
Echo server ( examples/echo_server.c )

  mrloop    288,169 responses/sec
  epoll     191,011 responses/sec 

```

A key value store https://github.com/MarkReedZ/mrcache

```
10B gets per second
  mrcache (io_uring)  5.7m
  redis               1.3m
  memcached           700k
```

# Usage

A simple timer.  See more code in examples/

```
#include "mrloop.h"

static mr_loop_t *loop = NULL;
static int cnt = 0;

// Return 0 to stop the timer
int on_timer( void *user_data ) {
    printf("tick\n");
    if ( ++cnt > 5 ) {
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
```
