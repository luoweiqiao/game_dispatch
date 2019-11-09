#!/bin/bash

rm CMakeCache.txt -f

rm -rf cmake_install.cmake CMakeCache.txt Makefile ./CMakeFiles
ls -l ./

cmake ./

while getopts rR opt
do
	case $opt in
		r)
			make clean
			;;
		R)
			make clean
			;;
		*)
			;;
	esac
done

make -j 4

cd $root_dir