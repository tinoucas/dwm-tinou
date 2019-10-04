#!/bin/zsh

BGIMAGE="$(grep '^feh\|^/' ~/.fehbg | sed "s/^[^']\+'\(.*\)' *$/\1/")"

ONEDRIVE="$(mount | grep OneDrive | sed 's#^[^/]*/\([^ ]*\) .*$#/\1#')"

if [ -n "$ONEDRIVE" ] && [ -n "$(echo $BGIMAGE | grep "^$ONEDRIVE")" ]; then
	~/hacks/scripts/setAsWallpaper.sh "$BGIMAGE" && exit 0
fi

DWMCOLORS=~/.config/dwm/colors
XRESOURCES=~/.Xresources
DUNSTRC=~/.config/dunst/dunstrc
COLORS="$(~/bin/colorart -s "0.628" -F 'normbg "%b", normfg "%d", selbg "%d", selfg "%b";%p;%d;%p;%b' "$BGIMAGE")"
echo $COLORS | cut -d\; -f1 > $DWMCOLORS
GHOSTFGCOLOR="$(echo $COLORS | cut -d\; -f2)"
CLOCKCOLOR="$(echo $COLORS | cut -d\; -f3)"

DUNSTFGCOLOR="$(echo $COLORS | cut -d\; -f4)"
DUNSTBGCOLOR="$(echo $COLORS | cut -d\; -f5)"

updateXresources()
{
	CLOCKCOLOR=$1
	GHOSTFGCOLOR=$1

	ed -s $XRESOURCES <<EOF
g/oclock\*BorderColor:/s/:\([	 ]*\).*$/:\1$CLOCKCOLOR/
g/oclock\*minute:/s/:\([	 ]*\).*$/:\1$CLOCKCOLOR/
.
wq
EOF
	if [ -n "$(grep "ghost_terminal\*foreground:" $XRESOURCES)" ]; then
		ed -s $XRESOURCES <<EOF
g/ghost_terminal\*foreground:/s/:\([	 ]*\).*$/:\1$GHOSTFGCOLOR/
wq
EOF
	elif [ -n "$(grep "ghost_terminal" $XRESOURCES)" ]; then
		ed -s $XRESOURCES <<EOF
/ghost_terminal/i
ghost_terminal*foreground:			$GHOSTFGCOLOR
.
wq
EOF
	else
		cat >> $XRESOURCES <<EOF

!urxvt as ghost_terminal
ghost_terminal*foreground:			$GHOSTFGCOLOR
ghost_terminal*background:			rgba:0000/0000/0000/0000
ghost_terminal*depth:				32
ghost_terminal*scrollBar:			false
ghost_terminal*cursorBlink:			true
EOF
	fi
	xrdb -merge $XRESOURCES
}

updateDunstColors()
{
	DUNSTFGCOLOR=$1
	DUNSTBGCOLOR=$2

	ed -s $DUNSTRC << EOF
/^ *\[urgency_normal/,/^ *\[urgency_critical/s/\(background[^"]*"\)[^"]*"/\1$DUNSTBGCOLOR"
/^ *\[urgency_normal/,/^ *\[urgency_critical/s/\(foreground[^"]*"\)[^"]*"/\1$DUNSTFGCOLOR"
wq
EOF
	pkill dunst
	nohup dunst &
}

updateXresources $GHOSTFGCOLOR
updateDunstColors $DUNSTFGCOLOR $DUNSTBGCOLOR
