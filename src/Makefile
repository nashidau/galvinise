
PREFIX=${HOME}

PKGS=			\
	talloc		\
	luajit

CFLAGS+=`pkg-config --cflags ${PKGS}`
LDFLAGS+=`pkg-config --libs ${PKGS}`


SRCS= 			\
	galv.o		\
	diskslurp.o	\
	blam_direct.o	\
	blam_writev.o	\
	modules/css/colours.o

galv: ${SRCS}
	${CC} -std=c99 ${LDFLAGS} -o galv ${SRCS}

install: galv
	cp galv ${PREFIX}/bin

clean:
	rm -f *.o galv
