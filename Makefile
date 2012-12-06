
SRCS= 			\
	src/galv.o	\
	src/blam.o

LDFLAGS+=-ltalloc

galv: ${SRCS}
	${CC} ${LDFLAGS} -o galv ${SRCS}
