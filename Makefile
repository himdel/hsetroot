INSTALL_PREFIX=/usr/local/bin
VERBOSE=@
CC?=gcc
PKG_CONFIG?=pkg-config

CFLAGS?=-g -O2 -Wall

CFLAGS_EXTRA=$(shell $(PKG_CONFIG) x11 --cflags) $(shell $(PKG_CONFIG) imlib2 --cflags)

LDFLAGS?=
LDFLAGS+=$(shell $(PKG_CONFIG) x11 --libs) $(shell $(PKG_CONFIG) imlib2 --libs)

BINARY=hsetroot
OBJS=$(BINARY).o outputs.o

$(BINARY): $(OBJS)
	$(VERBOSE)$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $^ -o $@ $(LDFLAGS)

%.o: %.c %.h
	$(VERBOSE)$(CC) -c $(CFLAGS) $(CFLAGS_EXTRA) $< -o $@

install: hsetroot
	$(VERBOSE)install -st $(INSTALL_PREFIX) $(BINARY)

clean:
	$(VERBOSE)rm -f hsetroot *.o
