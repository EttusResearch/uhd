#!/bin/bash

if [ $# -ne 3 ] && [ $# -ne 5 ];
then
	echo "Usage:"
	echo ""
	echo "writeprom.sh deviceid rev fab_rev [envvar envsetting]"
	echo	
	echo " deviceid   - expansion board device number from table:"
	echo 
	echo "   Summit     0x01"
	echo "   Tobi       0x02"
	echo "   Tobi Duo   0x03"
	echo "   Palo35     0x04"
	echo "   Palo43     0x05"
	echo "   Chestnut43 0x06"
	echo "   Pinto      0x07"
	echo
	echo " rev          - board revision (e.g. 0x00)"
	echo " fab_rev      - revision marking from pcb (e.g. R2411)"
	echo " envvar       - optional u-boot env variable name"
	echo "                (e.g. dvimode)"
	echo " envsetting   - optional u-boot env variable setting"
	echo "                (e.g. 1024x768MR-16@60)"
	exit 1
fi

fabrevision=$3
if [ ${#fabrevision} -ge 8 ]; then
	echo "Error: fab revision string must less than 8 characters"
	exit 1
fi

envvar=$4
if [ ${#envar} -ge 16 ]; then
	echo "Error: environment variable name string must less than 16 characters"
	exit 1
fi

envsetting=$5
if [ ${#ensetting} -ge 64 ]; then
	echo "Error: environment setting string must less than 64 characters"
	exit 1
fi

bus=3
device=0x51
vendorid=0x03

i2cset -y $bus $device 0x00 0x00
i2cset -y $bus $device 0x01 $vendorid
i2cset -y $bus $device 0x02 0x00
i2cset -y $bus $device 0x03 $1
i2cset -y $bus $device 0x04 $2
i2cset -y $bus $device 0x05 00

let i=6
hexdumpargs="'${#fabrevision}/1 \"0x%02x \"'"
command="echo -n \"$fabrevision\" | hexdump -e $hexdumpargs"
hex=$(eval $command)
for character in $hex; do
	i2cset -y $bus $device $i $character
	let i=$i+1
done
i2cset -y $bus $device $i 0x00

if [ $# -eq 5 ]
then
	i2cset -y $bus $device 0x05 0x01

	let i=14
	hexdumpargs="'${#envvar}/1 \"0x%02x \"'"
	command="echo -n \"$envvar\" | hexdump -e $hexdumpargs"
	hex=$(eval $command)
	for character in $hex; do
		i2cset -y $bus $device $i $character
		let i=$i+1
	done
 	i2cset -y $bus $device $i 0x00

	let i=30
	hexdumpargs="'${#envsetting}/1 \"0x%02x \"'"
	command="echo -n \"$envsetting\" | hexdump -e $hexdumpargs"
	hex=$(eval $command)
	for character in $hex; do
		i2cset -y $bus $device $i $character
		let i=$i+1
	done	 
	i2cset -y $bus $device $i 0x00
fi


