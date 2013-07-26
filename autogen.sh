#!/bin/bash
have_automake=false

if automake --version < /dev/null > /dev/null 2>&1 ; then
	automake_version=`automake --version | grep 'automake (GNU automake)' | sed 's/^[^0-9]*\(.*\)/\1/'`
	case $automake_version in
	   1.2*|1.3*|1.4|1.4*|1.5*|1.6*|1.7*|1.8*)
		;;
	   *)
		have_automake=true
		;;
	esac
fi
if $have_automake ; then
	AUTOMAKE="automake"
	ACLOCAL="aclocal"
else
	if automake-1.9 --version < /dev/null > /dev/null 2>&1 ; then
		AUTOMAKE="automake-1.9"
		ACLOCAL="aclocal-1.9"
	else
		echo "automake missing or too old. This requires at least automake 1.9"
		exit 1
	fi
fi

# ------ START GETTEXT ------

echo searching for GNU gettext intl directory...

dirs="/usr/share /usr/local/share /opt/share /usr /usr/local /opt /usr/gnu/share /opt/local /opt/local/share"
found=0
for try in $dirs; do
	echo -n " -> $try/gettext/intl... "
	if test -d $try/gettext/intl; then
		echo found it
		found=1
		break
	fi
	echo no
done
if test "$found" != 1; then
	echo ERROR: Cannot find gettext/intl directory.
	echo ERROR: Install GNU gettext in /usr or /usr/local prefix.
	exit 7
fi;

echo copying gettext intl files...
if test -f $try/gettext/mkinstalldirs; then
	ln -s $try/gettext/mkinstalldirs mkinstalldirs
fi
intldir="$try/gettext/intl"
if test ! -d intl; then
	mkdir intl
fi
olddir=`pwd`
cd $intldir
for file in *; do
	if test $file != COPYING.LIB-2.0 && test $file != COPYING.LIB-2.1; then
		rm -f $olddir/intl/$file
		cp $intldir/$file $olddir/intl/
	fi
done
cp -f $try/gettext/po/Makefile.in.in $olddir/po/
cd $olddir
if test -f intl/plural.c; then
	sleep 2
	touch intl/plural.c
fi

# ------ END GETTEXT ------


echo running $ACLOCAL...
$ACLOCAL $ACLOCAL_FLAGS
if test "$?" != "0"; then
	echo aclocal failed, stopping.
	exit 2
fi
echo running libtoolize...
libtoolize --copy --force --install
if test "$?" != "0"; then
	echo libtoolize failed, stopping.
	exit 3
fi
echo running autoheader...
autoheader
if test "$?" != "0"; then
	echo autoheader failed, stopping.
	exit 4
fi
echo running $AUTOMAKE...
$AUTOMAKE -a -c
if test "$?" != "0"; then
	echo automake failed, stopping.
	exit 5
fi
echo running autoconf...
autoconf
if test "$?" != "0"; then
	echo autoconf failed, stopping.
	exit 6
fi

echo if no errors occured, run ./configure
exit 0

#autogen.sh generates:
#	aclocal.m4 Makefile.in config.guess config.sub ltmain.sh
#	configure install-sh missing mkinstalldirs depcomp
#
#configure generates:
#	config.status libtool Makefile.in
