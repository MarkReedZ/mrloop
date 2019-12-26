
# MrLoop

Event loop for C using [io_uring](https://github.com/axboe/liburing) which requires linux kernel 5.4.1+

# Build

```
./bld
sudo ./bld install
```

# Benchmarks

Echo server benchmarked with https://github.com/haraldh/rust_echo_bench

```
Echo server ( examples/echo_server.c )

  mrloop    288,169 responses/sec
  epoll     191,011 responses/sec 

```

https://github.com/MarkReedZ/mrcache

```
10B gets per second
  mrcache (iouring) 4.6m
  redis             1.2m
  memcached         700k
```

# Usage

A simple timer.  See more code in examples/

```
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
  mr_add_timer(loop, 1, on_timer);
  mr_run(loop);
  mr_free(loop);

}
```
