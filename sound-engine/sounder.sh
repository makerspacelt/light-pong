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

		if [ $dir == 6 ]; then
			continue
		fi

		if [ ! -z $soundFile ]; then			
			play -q -v "1" "$dir/$soundFile" &
		fi

	done
}

play -q -v "0.4" 6/* repeat 9999 &

while true; do
	if  nc -n -z -w 1 192.168.4.2 2048; then
		play -q chimes.wav &
		echo -n -e '\x03' | nc -n -w 2 -vv 192.168.4.2 2048 | soundEngine
	else
		echo "No connection"
		sleep 1

	fi
done

sleep 1

