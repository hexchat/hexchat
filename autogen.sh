#!/bin/bash
have_automake=false
if automake --version < /dev/null > /dev/null 2>&1 ; then
	automake_version=`automake --version | grep 'automake (GNU automake)' | sed 's/^[^0-9]*\(.*\)/\1/'`
	case $automake_version in
	   1.2*|1.3*|1.4|1.4*)
		;;
	   *)
		have_automake=true
		;;
	esac
fi
if $have_automake ; then : ; else
	echo "automake missing or too old. This requires atleast automake 1.5"
	exit 1
fi

echo running aclocal...
aclocal
if test "$?" != "0"; then
	echo aclocal failed, stopping.
	exit 2
fi
echo running libtoolize...
libtoolize
if test "$?" != "0"; then
	echo libtoolize failed, stopping.
	exit 3
fi
echo running automake...
automake -a --foreign
if test "$?" != "0"; then
	echo automake failed, stopping.
	exit 4
fi
echo running autoconf...
autoconf
if test "$?" != "0"; then
	echo autoconf failed, stopping.
	exit 5
fi
echo if no errors occured, run ./configure --enable-maintainer-mode
exit 0

#autogen.sh generates:
#	aclocal.m4 Makefile.in config.guess config.sub ltmain.sh
#	configure install-sh missing mkinstalldirs depcomp
#
#configure generates:
#	config.status libtool Makefile.in
