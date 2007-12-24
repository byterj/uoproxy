# uoproxy Makefile
# (c) 2005-2007 Max Kellermann <max@duempel.org>
# $Id$

CC := gcc
PREFIX = /usr/local

# change this value to 'yes' to enable the debugging version
DEBUG = no

LDFLAGS = -levent

ifeq ($(DEBUG),yes)
CFLAGS += -g -O0
LDFLAGS += -g -O0
else
CFLAGS = -O3 -DNDEBUG=1
LDFLAGS += -O3
endif

#FEATURE_CFLAGS += -DDUMP_LOGIN
#FEATURE_CFLAGS += -DDUMP_USE

ifeq ($(DEBUG),yes)
WARNING_CFLAGS += -W -Wall -std=gnu99 -Wmissing-prototypes -Wwrite-strings -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare -Waggregate-return -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wno-long-long -Wstrict-prototypes -Wundef -pedantic-errors -Werror
else
WARNING_CFLAGS += -std=gnu99
endif

SOURCES = src/uoproxy.c src/config.c src/instance.c \
	src/daemon.c src/log.c \
	src/fifo-buffer.c src/flush.c src/sockbuff.c \
	src/buffered-io.c src/socket-util.c \
	src/netutil.c \
	src/server.c src/client.c \
	src/packets.c src/compression.c \
	src/pversion.c \
	src/cversion.c src/bridge.c \
	src/connection.c src/cclient.c src/cserver.c src/cnet.c src/world.c src/cworld.c src/walk.c \
	src/handler.c src/shandler.c src/chandler.c \
	src/attach.c src/reconnect.c \
	src/dump.c \
	src/sutil.c \
	src/command.c
HEADERS = $(wildcard src/*.h)

OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

.PHONY: all clean install strip dist upload rpm

all: src/uoproxy

clean:
	rm -f src/uoproxy src/*.o

$(OBJECTS): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(WARNING_CFLAGS) $(FEATURE_CFLAGS) -o $@ $<

src/uoproxy: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

install: src/uoproxy
	install -d -m 0755 /etc $(PREFIX)/bin
	test -f /etc/uoproxy.conf || install -m 0644 conf/uoproxy.conf /etc
	install -m 0755 src/uoproxy $(PREFIX)/bin

strip: src/uoproxy
	strip --strip-all $^

dist: VERSION := $(shell perl -ne 'print "$$1\n" if /^uoproxy \((.*?)\)/' NEWS |head -1)
dist:
	rm -rf /tmp/uoproxy
	mkdir -p /tmp/uoproxy
	svn export . /tmp/uoproxy/uoproxy-$(VERSION)
	cd /tmp/uoproxy && fakeroot tar cjf uoproxy-$(VERSION).tar.bz2 uoproxy-$(VERSION)

upload:
	scp README NEWS max@swift:/var/www/gzipped/download/uoproxy/doc/
	ssh max@swift chmod a+rX -R /var/www/gzipped/download/uoproxy/doc/

rpm: VERSION := $(shell perl -ne 'print "$$1\n" if /^uoproxy \((.*?)\)/' NEWS |head -1)
rpm: REVISION := 1
rpm: dist
	scp /tmp/uoproxy/uoproxy-$(VERSION).tar.bz2 redrat:/usr/src/redhat/SOURCES/uoproxy-$(VERSION).tar.bz2
	scp rpm/uoproxy.spec redrat:/usr/src/redhat/SPECS/
	ssh -n redrat rpmbuild -bb /usr/src/redhat/SPECS/uoproxy.spec
	scp redrat:/usr/src/redhat/RPMS/i386/uoproxy-$(VERSION)-$(REVISION).i386.rpm /tmp/uoproxy/
