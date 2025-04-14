#!/bin/sh

if [ ! -d /usr/local/m68k-elf/bin ]; then
	echo "No such destination dir as /usr/local/m68k-elf/bin"
	exit 1
fi

rsync -t mac11 mac65 mac68 mac68k mac69 mac8080 macas macpp mactj /usr/local/m68k-elf/bin

