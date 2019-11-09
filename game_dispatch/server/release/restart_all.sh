#!/bin/bash

root_dir=`pwd`
./stop_all.sh

cd ./dispatch_server/
./restart_game.sh
cd $root_dir
sleep 2

cd ./robot_server/
./restart_game.sh
cd $root_dir
sleep 2

all_dir=("./bainiu_server/" "./two_people_majiang_server" "./slot_server"
		 "./land_server" "./war_server" "./paijiu_server"
		 "./dice_server" "./baccarat_server" "./fight_server" "./niuniu_server" "./texas_server" "./zajinhua_server" "./showhand_server" "./robniu_server")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	./restart_game.sh;
	sleep 1
	cd $root_dir;
done
cd $root_dir
sleep 2

cd ./lobby_server/
./restart_game.sh

cd $root_dir




