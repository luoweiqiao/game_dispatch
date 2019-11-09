#!/bin/bash

root_dir=`pwd`

all_dir=("./bainiu_server/" "./two_people_majiang_server" "./slot_server"
		 "./land_server" "./war_server" "./paijiu_server"
		 "./dice_server" "./baccarat_server" "./fight_server"
		 "./niuniu_server" "./texas_server" "./zajinhua_server" "./showhand_server" "./robniu_server"
		 "./lobby_server")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./stop.sh;
	cd $root_dir;
done

sleep 2 
cd ./robot_server/
./stop.sh
cd $root_dir

sleep 2
cd ./dispatch_server/
./stop.sh
cd $root_dir

sleep 2
cd ./lobby_server/
./stop.sh
cd $root_dir
