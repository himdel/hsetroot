CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

VERSION?=$(shell git describe --tags 2>/dev/null || echo 1.0.5)
CPPFLAGS+=-DVERSION=\"$(VERSION)\"

PREFIX?=/usr/local
DESTDIR?=

# arch hardening
#CPPFLAGS+=-D_FORTIFY_SOURCE=2
#CFLAGS+=-march=x86-64 -mtune=generic -O2 -pipe -fno-plt
#LDFLAGS+=-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now

# older imlib2 renders gradients wrong above 4096px, IMLIB2_MIN=0 to build anyway
IMLIB2_MIN?=1.12.6
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(shell $(PKG_CONFIG) --atleast-version=$(IMLIB2_MIN) imlib2 && echo ok),ok)
$(error imlib2 >= $(IMLIB2_MIN) required)
endif
endif

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

check: hsetroot
	sh tests/run.sh

clean:
	rm -f *.o hsetroot hsr-outputs
