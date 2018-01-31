CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

CFLAGS+=$(shell $(PKG_CONFIG) x11 --cflags)
LDFLAGS+=$(shell $(PKG_CONFIG) x11 --libs)

CFLAGS+=$(shell $(PKG_CONFIG) imlib2 --cflags)
LDFLAGS+=$(shell $(PKG_CONFIG) imlib2 --libs)

hsetroot: hsetroot.o outputs_xrandr.o

hsetroot.o: hsetroot.c outputs.h

outputs.o: outputs.c outputs.h

install: hsetroot
	install -st /usr/local/bin/ hsetroot

clean:
	rm -f hsetroot *.o
