#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/src/common/hexchat.c) || {
	echo -n "**Error**: Directory "\`$srcdir\'" does not look like the top-level directory"
	exit 1
}

if [ "$1" = "--copy" ]; then
	shift
	aclocal --force --install || exit 1
	intltoolize --force --copy --automake || exit 1
	autoreconf --force --install --include=m4 -Wno-portability || exit 1
else
	intltoolize --automake || exit 1
	autoreconf --install --symlink --include=m4 -Wno-portability || exit 1
fi

if [ "$NOCONFIGURE" = "" ]; then
        $srcdir/configure "$@" || exit 1

        if [ "$1" = "--help" ]; then exit 0 else
                echo "Now type \`make\' to compile" || exit 1
        fi
else
        echo "Skipping configure process."
fi

set +x
