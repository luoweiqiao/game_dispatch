#!/bin/bash

root_dir=`pwd`

all_dir=("./dispatch_server/" "./lobby_server/" "./robot_server/"
		 "./bainiu_server/" "./two_people_majiang_server/" "./slot_server/"
         "./land_server/" "./war_server/" "./paijiu_server/"
		 "./dice_server/" "./baccarat_server/" "./fight_server/"
		 "./niuniu_server/" "./texas_server/" "./zajinhua_server/" "./showhand_server/" "./robniu_server/")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	cd $dirname
	echo `pwd`
	ls -l ./
	cd $root_dir;
done