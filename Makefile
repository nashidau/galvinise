# Travis compatible Makefile
# ./configure && make && make test

all:
	make -C src

clean:
	make -C src clean

test:
	make -C src check
	(cd src && ./check)

data/galvinise.pc: data/galvinise.pc.in galv
	galv 

install:
