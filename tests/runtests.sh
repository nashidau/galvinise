#!/usr/bin/env zsh
set -e


for i in test???.gvz ; do
	../src/galv $i && diff -u `basename $i .gvz`.expect `basename $i .gvz`
done

#for i in testF???.gvz ; do
#	../src/galv $i
#done
