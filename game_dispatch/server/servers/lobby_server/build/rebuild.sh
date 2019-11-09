#!/bin/bash

rm CMakeCache.txt -f
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

rm -f ../../../release/robot_server/robotServer
cp ../../../release/lobby_server/lobbyServer ../../../release/robot_server/robotServer

rm -f ../../../release/lobby_server2/lobbyServer
cp ../../../release/lobby_server/lobbyServer ../../../release/lobby_server2/lobbyServer

cd $root_dir
