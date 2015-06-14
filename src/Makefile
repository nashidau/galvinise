
PREFIX=/usr/local

PKGS=			\
	talloc		\
	luajit

# FIXME: No .pc file for onion

ifndef CFLAGS
CFLAGS=-Wall -O2 -g3
endif

CFLAGS+=`pkg-config --cflags ${PKGS}`
LDFLAGS+=`pkg-config --libs ${PKGS}` -lonion

SRCS= 			\
	galv.o		\
	diskslurp.o	\
	blam_direct.o	\
	blam_writev.o	\
	blam_onion.o	\
	modules/css/colours.o

galv: main.o libgalv.a
	${CC} ${CFLAGS} -std=c99 -o galv main.o -L. -lgalv ${LDFLAGS}

libgalv.a : ${SRCS}
	rm -f libgalv.a
	ar rcs libgalv.a ${SRCS}

install: libgalv.a galv
	cp galv ${PREFIX}/bin
	install galvinise.h /usr/local/include
	install libgalv.a /usr/local/lib

clean:
	rm -f *.o **/*.o galv
