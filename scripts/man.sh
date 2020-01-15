#!/bin/bash
shopt -s -o nounset

count=0
W=()
_script="$(readlink -f ${BASH_SOURCE[0]})"
_dir="$(dirname $_script)/../Documentation/services"
filenames=$(ls $_dir | grep -I "pcn-*")

#det current screen dimensions
read -r WHIP_SIZE_Y WHIP_SIZE_X < <(stty size)

for filename in $filenames
do
	W+=($count "$filename")	
	let "count++"
done

CONTENT_SIZE=$((WHIP_SIZE_Y-9))
OPTION=$(whiptail --title "Welcome to Polycube documentation" --menu "Choose a service. Full documentation at https://polycube-network.readthedocs.io/en/latest/index.html#" $WHIP_SIZE_Y $WHIP_SIZE_X $CONTENT_SIZE "${W[@]}" 3>&1 1>&2 2>&3)

exitstatus=$?
if [ $exitstatus = 0 ]; then
	count=0
	for dir in ${filenames[@]}
	do
		if [ $count = $OPTION ]; then
			whiptail --textbox --scrolltext ${_dir}/${dir}/${dir:4}.rst $WHIP_SIZE_Y $WHIP_SIZE_X
			break
		else
			let "count++"
		fi
	done
fi
