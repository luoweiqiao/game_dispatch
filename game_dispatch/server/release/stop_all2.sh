#!/bin/bash

root_dir=`pwd`

all_dir=("lobby_server2")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./stop.sh;
	cd $root_dir;
done

cd $root_dir

