CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

PREFIX?=/usr/local
DESTDIR?=

# arch hardening
#CPPFLAGS+=-D_FORTIFY_SOURCE=2
#CFLAGS+=-march=x86-64 -mtune=generic -O2 -pipe -fno-plt
#LDFLAGS+=-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now


CFLAGS+=$(shell $(PKG_CONFIG) x11 --cflags)
LDLIBS+=$(shell $(PKG_CONFIG) x11 --libs)

CFLAGS+=$(shell $(PKG_CONFIG) imlib2 --cflags)
LDLIBS+=$(shell $(PKG_CONFIG) imlib2 --libs)

CFLAGS+=$(shell $(PKG_CONFIG) xinerama --cflags)
LDLIBS+=$(shell $(PKG_CONFIG) xinerama --libs)

all: hsetroot hsr-outputs

hsetroot: hsetroot.o

hsr-outputs: hsr-outputs.o

install: hsetroot hsr-outputs
	install -Dst $(DESTDIR)$(PREFIX)/bin/ hsetroot
	install -Dst $(DESTDIR)$(PREFIX)/bin/ hsr-outputs

clean:
	rm -f *.o hsetroot hsr-outputs
