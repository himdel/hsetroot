CC=gcc
CFLAGS=-g -O2 -Wall
LDFLAGS=-lImlib2 -lX11

hsetroot: hsetroot.o

hsetroot.o: hsetroot.c

clean:
	rm -f hsetroot hsetroot.o
