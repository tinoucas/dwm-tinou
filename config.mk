# dwm version
VERSION = 6.0

# Customize below to fit your system

# paths
PREFIX = /usr
MANPREFIX = ${PREFIX}/share/man

# includes and libs
XFTFLAGS = `pkg-config --cflags pangocairo`
X11FLAGS = `pkg-config --cflags x11-xcb xcb-res xinerama` -DXINERAMA

XFTLIBS = `pkg-config --libs pangocairo`
X11LIBS = `pkg-config --libs x11-xcb xcb-res xinerama`

LIBS = -lc ${X11LIBS} ${XFTLIBS}

SYMBOLSCFLAGS = -g
SYMBOLSLDFLAGS = -g
#SYMBOLSCFLAGS =
#SYMBOLSLDFLAGS = -s

# flags
VERSIONCFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS = ${SYMBOLSCFLAGS} -std=c99 -pedantic -Wall -O0 -I. -I/usr/include ${XFTFLAGS} ${X11FLAGS} ${VERSIONCFLAGS} -Wno-deprecated-declarations -Wno-parentheses
LDFLAGS = ${SYMBOLSLDFLAGS} ${LIBS}

# compiler and linker
CC = cc
