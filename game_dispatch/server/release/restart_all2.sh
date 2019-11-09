#!/bin/bash

root_dir=`pwd`
./stop_all2.sh

all_dir=("lobby_server2")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./restart_game.sh;
	cd $root_dir;
done
cd $root_dir



