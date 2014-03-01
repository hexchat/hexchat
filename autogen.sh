#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

NOCONFIGURE=1
PKG_NAME="hexchat"

(test -f $srcdir/src/common/hexchat.c) || {
	echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
	echo " top-level $PKG_NAME directory"
	exit 1
}

which gnome-autogen.sh || {
	echo "You need to install gnome-common"
	exit 1
}

. gnome-autogen.sh

