
# MrLoop

Event loop for C using [io_uring](https://github.com/axboe/liburing) which requires linux kernel 5.3+

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

