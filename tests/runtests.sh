#!/usr/bin/zsh
set -e


for i in test???.gvz ; do
	../src/galv $i && diff -u `basename $i .gvz` `basename $i .gvz`.expect
done

for i in testF???.gvz ; do
	../src/galv $i
done
