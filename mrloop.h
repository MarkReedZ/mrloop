
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

#define MAX_CONN 1024


typedef struct event event_t;
typedef void writeCB(void *user_data);

typedef struct mrLoop {
  int stop;
  struct io_uring *ring;
  event_t *readEvents[MAX_CONN];
  event_t *readDataEvents[MAX_CONN];
  //event_t *writeDataEvents[MAX_CONN];
  event_t *writeDataEvent;
  writeCB *writeCallback;
  //struct iovec iovs[MAX_CONN];
} mrLoop;

typedef void timerCB();
typedef void *acceptCB(int fd, char **buf, int *buflen);
typedef void readCB(void *conn, int fd, ssize_t nread, char *buf);
typedef void sigCB(int sig);

typedef struct event {
  int type;
  int fd;
  timerCB *tcb;  // TODO union
  acceptCB *acb;  
  readCB *rcb;
  //writeCB *wcb;
  void *user_data;

  struct iovec iov;

} event_t;

mrLoop *createLoop();
void freeLoop(mrLoop *loop);
void stopLoop(mrLoop *loop);
void runLoop(mrLoop *loop);

int addTimer( mrLoop *loop, int seconds, timerCB *cb );
int mrTcpServer( mrLoop *loop, int port, acceptCB *cb, readCB *rcb);//, char *buf, int buflen );
int mrConnect( mrLoop *loop, char *addr, int port, readCB *rcb);

void mrFlush(mrLoop *loop);
void mrWrite( mrLoop *loop, int fd, char *buf, int buflen );
void mrWritev( mrLoop *loop, int fd, struct iovec *iovs, int cnt );
void mrWritevf( mrLoop *loop, int fd, struct iovec *iovs, int cnt );

//void mrWritev2( mrLoop *loop, int fd, struct iovec *iovs, int cnt, void *user_data );
