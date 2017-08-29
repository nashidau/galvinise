# Travis compatible Makefile
# ./configure && make && make test

all:
	make -C src

test:
	make -C src check
	(cd src && ./check)
