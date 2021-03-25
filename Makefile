# Travis compatible Makefile
# ./configure && make && make test

include config.mk

all:
	make -C src

check: test

clean:
	make -C src clean

test:
	make -C src check
	(cd src && ./check)

data/galvinise.pc: data/galvinise.pc.in galv
	galv 

install:
	make -C src install
