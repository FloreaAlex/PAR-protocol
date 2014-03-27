#!/bin/bash

SPEED=1
DELAY=1
LOSS=40
CORRUPT=40

if [ "$#" -eq 1 ]
then
killall link
killall recv
killall send

./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT &> /dev/null &
sleep 1
./recv "$1" &
sleep 1
./send "$1"
else
	echo "USAGE: ./run_experiment <input_file_name>"
fi

echo "Finished transfer, checking files"