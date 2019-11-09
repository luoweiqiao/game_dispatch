#!/bin/bash

root_dir=`pwd`

all_dir=("./dispatch_server/dispatchServer" "./lobby_server/lobbyServer" "./robot_server/robotServer"
		 "./bainiu_server/bainiuServer" "./two_people_majiang_server/majiangServer" "./slot_server/slotServer"
         "./land_server/landServer" "./war_server/warServer" "./paijiu_server/paijiuServer"
		 "./dice_server/diceServer" "./baccarat_server/baccaratServer" "./fight_server/fightServer"
		 "./majiang_server/majiangServer" "./lobby_server2/lobbyServer" "./niuniu_server/niuniuServer"
		 "./texas_server/texasServer" "./zajinhua_server/zajinhuaServer" "./showhand_server/showhandServer" "./robniu_server/robniuServer")

tLen=${#all_dir[@]}
for ((i=0;i<${tLen};i++))
do
	dirname=${all_dir[$i]}
	rm -f $dirname
done





