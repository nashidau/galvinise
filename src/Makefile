

PKGS=			\
	talloc		\
	luajit

CFLAGS+=`pkg-config --cflags ${PKGS}`
LDFLAGS+=`pkg-config --libs ${PKGS}`


SRCS= 			\
	galv.o	\
	blam.o

galv: ${SRCS}
	${CC} -std=c99 ${LDFLAGS} -o galv ${SRCS}

clean:
	rm -f *.o galv
