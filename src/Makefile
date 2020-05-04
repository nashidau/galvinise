
PREFIX=/usr/local

PKGS=			\
	talloc		\
	lua5.1

# FIXME: No .pc file for onion

.PHONY:tags

ifndef CFLAGS
CFLAGS=-Wall -O2 -g3
endif

include ../config.mk

USE_ONION ?= n
ifeq (${USE_ONION}, y)
ONION_LDFLAGS=-lonion
ONION_CFLAGS=-DUSE_ONION=1
else
ONION_CFLAGS=-DUSE_ONION=0
endif


CFLAGS+=`pkg-config --cflags ${PKGS}` ${ONION_CFLAGS}
CHECK_CFLAGS+=`pkg-config --cflags ${PKGS} check` ${ONION_CFLAGS}
LDFLAGS+=`pkg-config --libs ${PKGS}` ${ONION_LDFLAGS}
CHECK_LDFLAGS+=`pkg-config --libs ${PKGS} check` ${ONION_LDFLAGS}

srcs-y=galv.o
srcs-y+=diskslurp.o
srcs-y+=blam_buf.o
srcs-y+=blam_direct.o
srcs-y+=blam_writev.o
srcs-y+=modules/css/colours.o
srcs-${USE_ONION}+= blam_onion.o

TESTS=			\
	galv_check.o	\
	blam_check.o	\
	check.o

galv: main.o libgalv.a
	${CC} ${CFLAGS} -std=c99 -o galv main.o -L. -lgalv ${LDFLAGS}

check: libgalv.a ${TESTS}
	${CC} ${CHECK_CFLAGS} -o check ${TESTS} -L. -lm -lgalv ${CHECK_LDFLAGS}
	

libgalv.a : ${srcs-y}
	rm -f libgalv.a
	ar rcs libgalv.a ${srcs-y}

install: libgalv.a galv
	cp galv ${PREFIX}/bin
	install galvinise.h /usr/local/include
	install libgalv.a /usr/local/lib

tags:
	ctags -R .

clean:
	rm -f *.o **/*.o galv
