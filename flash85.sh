#!/bin/bash

if [ -z $1 ]
then
	echo "Usage: ./flash85.sh file.c"
	exit 1
fi

FILENAME=${1%.*}

if [ -z $FILENAME ]
then
	echo "Usage: flash85 file.c"
	exit 1
fi

avr-gcc -Os -DF_CPU=1000000UL -mmcu=attiny85 -c -o "$FILENAME.o" "$1" || exit 1
avr-gcc -mmcu=attiny85 "$FILENAME.o" -o "$FILENAME" || exit 1
avr-objcopy -O ihex -R .eeprom "$FILENAME" "$FILENAME.hex" || exit 1

while true
do
	echo "Flash? Y/N"
			
	read sel

	if [ $sel == "Y"  ] || [ $sel == "y"  ]
	then
		break
	elif [ $sel == "N"  ] || [ $sel == "n"  ]
	then
		exit 0
	fi
done

avrdude -v -c usbasp -p t85 -U flash:w:$FILENAME.hex:i -B 8
	
