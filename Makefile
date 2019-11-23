
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

LIBS = -luring
CC = gcc
CFLAGS = -O3

.PHONY: default all clean

default: libmrloop.a
all: default

mrloop.o: mrloop.c
	$(CC) $(CFLAGS) -c mrloop.c -o mrloop.o $(LIBS)

libmrloop.a: mrloop.o
	ar crs libmrloop.a mrloop.o 

clean:
	-rm -f *.o

install: libmrloop.a
	install -d $(PREFIX)/lib/
	install -m 644 libmrloop.a $(PREFIX)/lib/
	install -d $(PREFIX)/include/
	install -m 644 mrloop.h $(PREFIX)/include/
