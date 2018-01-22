CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

CFLAGS+=`$(PKG_CONFIG) x11 --cflags`
LDFLAGS+=`$(PKG_CONFIG) x11 --libs`

CFLAGS+=`$(PKG_CONFIG) imlib2 --cflags`
LDFLAGS+=`$(PKG_CONFIG) imlib2 --libs`

hsetroot: hsetroot.o outputs_xrandr.o

hsetroot.o: hsetroot.c outputs.h

outputs.o: outputs.c outputs.h

install: hsetroot
	install -st /usr/local/bin/ hsetroot

clean:
	rm -f hsetroot *.o
