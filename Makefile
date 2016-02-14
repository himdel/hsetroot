CC=gcc

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

CFLAGS+=`pkg-config x11 --cflags`
LDFLAGS+=`pkg-config x11 --libs`

CFLAGS+=`pkg-config imlib2 --cflags`
LDFLAGS+=`pkg-config imlib2 --libs`

hsetroot: hsetroot.o outputs_xrandr.o

hsetroot.o: hsetroot.c outputs.h

outputs.o: outputs.c outputs.h

install: hsetroot
	install -st /usr/local/bin/ hsetroot

clean:
	rm -f hsetroot *.o
