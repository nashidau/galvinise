
PREFIX=/usr/local

PKGS=			\
	talloc		\
	luajit

# FIXME: No .pc file for onion

.PHONY:tags

ifndef CFLAGS
CFLAGS=-Wall -O2 -g3
endif

CFLAGS+=`pkg-config --cflags ${PKGS}`
CHECK_CFLAGS+=`pkg-config --cflags ${PKGS} check`
LDFLAGS+=`pkg-config --libs ${PKGS}` -lonion
CHECK_LDFLAGS+=`pkg-config --libs ${PKGS} check` -lonion

SRCS= 			\
	galv.o		\
	diskslurp.o	\
	blam_buf.o	\
	blam_direct.o	\
	blam_writev.o	\
	blam_onion.o	\
	modules/css/colours.o

TESTS=			\
	blam_check.o

galv: main.o libgalv.a
	${CC} ${CFLAGS} -std=c99 -o galv main.o -L. -lgalv ${LDFLAGS}

check: libgalv.a ${TESTS}
	${CC} ${CHECK_CFLAGS} -o check ${TESTS} -L. -lm -lgalv ${CHECK_LDFLAGS}
	

libgalv.a : ${SRCS}
	rm -f libgalv.a
	ar rcs libgalv.a ${SRCS}

install: libgalv.a galv
	cp galv ${PREFIX}/bin
	install galvinise.h /usr/local/include
	install libgalv.a /usr/local/lib

tags:
	ctags -R .

clean:
	rm -f *.o **/*.o galv
