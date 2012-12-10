#!/usr/bin/zsh



for i in *.gvz ; do
	../src/galv $i && diff -u `basename $i .gvz` `basename $i .gvz`.expect
done

