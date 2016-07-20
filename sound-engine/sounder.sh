#!/bin/bash
set -e
set -a

cd "$(dirname $0)"

soundEngine() {
	while IFS= read -r -d'|' char
	do
		echo $char | xxd
		if [[ "$(printf "%d" "'${char:0:1}")" != "4" ]]; then
			continue
		fi

		dir=$(printf "%d" "'${char:1:1}")
		echo "DIR: $dir"		
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
		echo -n -e '\x03' | nc -n -vv 192.168.4.2 2048 | soundEngine
	else
		echo "No connection"
		sleep 1

	fi
done

sleep 1

