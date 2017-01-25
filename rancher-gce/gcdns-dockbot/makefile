#
#FILE
#	docker-odoo-adhoc/rancher-gce/gcdns-dockbot/makefile
#AUTHOR/LEGAL
#	(C) 2017 Gary Wallis for AdHoc Ing S.A.
#NOTES
#	See LICENSE in source tree
#

GIT_VERSION := $(shell git describe --dirty --always --tags)
CFLAGS+=-Wall -DGitVersion=\"$(GIT_VERSION)\"
#LDFLAGS+=`curl-config --libs`
LDFLAGS+=/usr/lib/x86_64-linux-gnu/libcurl-gnutls.so.4

TARGETS=dockbot
OBJS=buf.o log.o json.o jsmn.o

all: $(TARGETS) .gitignore

clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)

dockbot: $(OBJS) dockbot.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: all clean
