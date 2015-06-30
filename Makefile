CC=gcc
CFLAGS=-g -O2 -Wall `pkg-config x11 --cflags` `pkg-config imlib2 --cflags`
LDFLAGS=`pkg-config x11 --libs` `pkg-config imlib2 --libs`

hsetroot: hsetroot.o

hsetroot.o: hsetroot.c

clean:
	rm -f hsetroot hsetroot.o
