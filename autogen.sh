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

#autogen.sh generates:
#	aclocal.m4 Makefile.in config.guess config.sub ltmain.sh
#	configure install-sh missing mkinstalldirs depcomp
#
#configure generates:
#	config.status libtool Makefile.in
