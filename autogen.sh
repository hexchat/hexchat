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
echo running libtoolize...
libtoolize
echo running automake in src...
automake -a --foreign Makefile
automake -a --foreign src/Makefile
automake -a --foreign src/common/Makefile
automake -a --foreign src/fe-gtk/Makefile
automake -a --foreign src/fe-text/Makefile
automake -a --foreign src/pixmaps/Makefile
echo running automake in plugins...
automake -a --foreign plugins/Makefile
automake -a --foreign plugins/tcl/Makefile
automake -a --foreign plugins/perl/Makefile
automake -a --foreign plugins/python/Makefile
echo running autoconf...
autoconf
echo if no errors occured, run ./configure --enable-maintainer-mode
exit 0

#autogen.sh generates:
#	aclocal.m4 Makefile.in config.guess config.sub ltmain.sh
#	configure install-sh missing mkinstalldirs depcomp
#
#configure generates:
#	config.status libtool Makefile.in
