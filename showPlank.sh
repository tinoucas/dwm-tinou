#!/bin/zsh

if [ -z "$1" ] || [ "$1" = "show" ]; then
	HIDEMODE="none"
elif [ "$1" = "hide" ]; then
	HIDEMODE="auto"
else
	echo "$0 [show|hide]"
	exit 3
fi

dconf dump /net/launchpad/plank/docks/ | sed "s/hide-mode='[^']*'/hide-mode='$HIDEMODE'/" | dconf load /net/launchpad/plank/docks/
