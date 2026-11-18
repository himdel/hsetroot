CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall
LDFLAGS?=

VERSION?=$(shell test -e .git && git describe --tags 2>/dev/null || echo 1.1.0)
CPPFLAGS+=-DVERSION=\"$(VERSION)\"

# color names without an X server
RGB_TXT?=/usr/share/X11/rgb.txt
CPPFLAGS+=-DRGB_TXT=\"$(RGB_TXT)\"

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

CFLAGS+=$(shell $(PKG_CONFIG) xrandr --cflags)
LDLIBS+=$(shell $(PKG_CONFIG) xrandr --libs)

# Wayland (wlr-layer-shell compositors), when libwayland-client is there, WAYLAND=0 to skip
WAYLAND?=$(shell $(PKG_CONFIG) --exists wayland-client wayland-protocols && echo 1)
ifeq ($(WAYLAND),1)
CPPFLAGS+=-DWAYLAND
CFLAGS+=$(shell $(PKG_CONFIG) wayland-client --cflags)
LDLIBS+=$(shell $(PKG_CONFIG) wayland-client --libs)
WAYLAND_OBJS=wayland.o wlr-layer-shell-unstable-v1-protocol.o xdg-shell-protocol.o xdg-output-unstable-v1-protocol.o
WLP=$(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)
endif

all: hsetroot hsr-outputs

wayland.o: wlr-layer-shell-unstable-v1-client-protocol.h xdg-output-unstable-v1-client-protocol.h

vpath xdg-shell.xml $(WLP)/stable/xdg-shell
vpath xdg-output-unstable-v1.xml $(WLP)/unstable/xdg-output

%-client-protocol.h: %.xml
	wayland-scanner client-header $< $@

%-protocol.c: %.xml
	wayland-scanner private-code $< $@

hsetroot: hsetroot.o outputs.o $(WAYLAND_OBJS)

hsr-outputs: hsr-outputs.o outputs.o

install: hsetroot hsr-outputs
	install -Dst $(DESTDIR)$(PREFIX)/bin/ hsetroot
	install -Dst $(DESTDIR)$(PREFIX)/bin/ hsr-outputs
	install -Dm644 -t $(DESTDIR)$(PREFIX)/share/man/man1/ hsetroot.1

check: hsetroot hsr-outputs
	sh tests/run.sh

clean:
	rm -f *.o hsetroot hsr-outputs *-protocol.[ch]
