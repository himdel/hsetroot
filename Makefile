CC=gcc
CFLAGS=-g -O2 -Wall
LDFLAGS=`pkg-config x11 --libs`
CFLAGS+=`pkg-config x11 --cflags`

CFLAGS+=`pkg-config imlib2 --cflags`
LDFLAGS+=`pkg-config imlib2 --libs`

CFLAGS+=`pkg-config xrandr --cflags`
LDFLAGS+=`pkg-config xrandr --libs`

hsetroot: hsetroot.o

hsetroot.o: hsetroot.c

clean:
	rm -f hsetroot hsetroot.o
