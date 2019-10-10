#!/bin/zsh

if [ -z "$1" ] || [ "$1" = "show" ]; then
	HIDEMODE="none"
elif [ "$1" = "hide" ]; then
	HIDEMODE="auto"
else
	exit 3
fi
shift
if [ "$1" = "0" ]; then
	MONITOR="HDMI-0"
	OFFSET="0"
elif [ "$1" = "1" ]; then
	MONITOR="DVI-I-1"
	OFFSET="-1"
else
	exit 3
fi

dconf dump /net/launchpad/plank/docks/ | sed "s/hide-mode='[^']*'/hide-mode='$HIDEMODE'/" | sed "s/monitor='[^']*'/monitor='$MONITOR'/" | dconf load /net/launchpad/plank/docks/
sleep 0.1
dconf dump /net/launchpad/plank/docks/ | sed "s/offset=-*[0-9]*$/offset=$OFFSET/" | dconf load /net/launchpad/plank/docks/
