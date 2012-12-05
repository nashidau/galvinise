
SRCS= 			\
	src/galv.o

LDFLAGS+=-ltalloc

galv: ${SRCS}
	${CC} ${LDFLAGS} -o galv ${SRCS}
