
#pragma once

#include <netinet/in.h>
#include <fcntl.h> 
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <sys/uio.h>

#include "liburing.h"

#define TIMER_EV 1
#define LISTEN_EV 2
#define READ_EV 3
#define READ_DATA_EV 4
#define WRITE_DATA_EV 5
#define WRITE_EV 6
#define TIMER_ONCE_EV 7

#define MAX_CONN 1024


typedef struct event_s event_t;
typedef void mr_write_cb(void *conn, int fd);
typedef void mr_write_done_cb(void *user_data);

typedef struct mr_loop_s {
  int stop;
  int fd;
  struct io_uring *ring;
  event_t *readEvents[MAX_CONN];
  event_t *readDataEvents[MAX_CONN];
  //event_t *writeEvents[MAX_CONN];
  //event_t *writeDataEvents[MAX_CONN];
  event_t *writeDataEvent;
  mr_write_cb *writeCallback;
  //struct iovec iovs[MAX_CONN];
} mr_loop_t;

typedef void mr_timer_cb();
typedef void *mr_accept_cb(int fd, char **buf, int *buflen);
typedef void mr_read_cb(void *conn, int fd, ssize_t nread, char *buf);
typedef void mr_signal_cb(int sig);

typedef struct event_s {
  int type;
  int fd;
  // TODO union
  mr_timer_cb *tcb;  
  mr_accept_cb *acb;  
  mr_read_cb *rcb;
  mr_write_cb *wcb;
  mr_write_done_cb *wdcb;
  void *user_data;

  struct iovec iov;

} event_t;

mr_loop_t *mr_create_loop();
void mr_free(mr_loop_t *loop);
void mr_stop(mr_loop_t *loop);
void mr_run(mr_loop_t *loop);

void mr_add_write_callback( mr_loop_t *loop, mr_write_cb *cb, void *conn, int fd );
int mr_add_timer( mr_loop_t *loop, double seconds, mr_timer_cb *cb );
int mr_tcp_server( mr_loop_t *loop, int port, mr_accept_cb *cb, mr_read_cb *rcb);//, char *buf, int buflen );
int mr_connect( mr_loop_t *loop, char *addr, int port, mr_read_cb *rcb);
void mr_close(mr_loop_t *loop, int fd);

void mr_flush(mr_loop_t *loop);
void mr_write( mr_loop_t *loop, int fd, const void *buf, unsigned nbytes, off_t offset );
void mr_writev( mr_loop_t *loop, int fd, struct iovec *iovs, int cnt );
void mr_writevf( mr_loop_t *loop, int fd, struct iovec *iovs, int cnt );
void mr_writevcb( mr_loop_t *loop, int fd, struct iovec *iovs, int cnt, void *user_data, mr_write_done_cb *cb );

void mr_call_after( mr_loop_t *loop, mr_timer_cb *func, int milliseconds );





