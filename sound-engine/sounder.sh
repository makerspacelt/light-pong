#!/bin/bash
set -e
set -a

cd "$(dirname $0)"

soundEngine() {
	while IFS= read -r -n1 char
	do
		dir=$(printf "%d" "'$char")
		soundFile=$(ls "$dir" | sort -R | tail -1)

		echo "Event: $dir"

		if [ $dir == 6 ] || [ $dir == 4 ]; then
			if [ ! -z $bgPid ]; then
				kill "$bgPid"
				unset bgPid
			fi
		fi


		if [ $dir == 6 ]; then
			volume="0.4"
		else
			volume="1"
		fi


		if [ ! -z $soundFile ]; then			
			play -q -v "$volume" "$dir/$soundFile" &
			if [ $dir == 6 ]; then
				bgPid=$!
				echo "BG: $bgPid"
			fi
		fi

	done
}

if  nc -z -w 1 192.168.4.1 2048; then
	play -q chimes.wav &
	echo -n -e '\x03' | nc -w 1 -vv 192.168.4.1 2048 | soundEngine
else
	echo "No connection"
	sleep 1
fi

sleep 1
