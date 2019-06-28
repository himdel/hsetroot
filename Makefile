CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

# arch hardening
#CPPFLAGS+=-D_FORTIFY_SOURCE=2
#CFLAGS+=-march=x86-64 -mtune=generic -O2 -pipe -fno-plt
#LDFLAGS+=-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now

# arch hardening workaround
# LDFLAGS+=-Wl,--no-as-needed

CFLAGS+=$(shell $(PKG_CONFIG) x11 --cflags)
LDFLAGS+=$(shell $(PKG_CONFIG) x11 --libs)

CFLAGS+=$(shell $(PKG_CONFIG) imlib2 --cflags)
LDFLAGS+=$(shell $(PKG_CONFIG) imlib2 --libs)

hsetroot: hsetroot.o outputs_xrandr.o

hsetroot.o: hsetroot.c outputs.h

outputs.o: outputs.c outputs.h

outputs_xrandr.o: outputs_xrandr.c outputs.h

install: hsetroot
	install -st /usr/local/bin/ hsetroot

clean:
	rm -f hsetroot *.o
