dnl Process this file with autoconf to produce a configure script.

AC_INIT([HexChat],[2.9.5])

AC_PREREQ([2.60])
AC_COPYRIGHT([Copyright (C) 1998-2010 Peter Zelezny])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 dist-bzip2 subdir-objects no-define foreign])
AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS
AM_MAINTAINER_MODE
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AM_PROG_AS
AM_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl -----------------------------------------------------------
dnl Language Support
dnl -----------------------------------------------------------
GETTEXT_PACKAGE=hexchat
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])

ALL_LINGUAS="af am ast az be bg ca cs da de el en_GB es et eu fi fr gl gu hi hu id it ja kn ko lt lv mk ml ms nb nl no pa pl pt pt_BR ru rw sk sl sq sr sv th uk vi wa zh_CN zh_TW"
AM_GNU_GETTEXT

dnl displaced from acconfig.h
AH_VERBATIM([OLD_PERL],[#undef OLD_PERL])
AH_VERBATIM([PREFIX],[#undef PREFIX])
AH_VERBATIM([HEXCHATLIBDIR],[#undef HEXCHATLIBDIR])
AH_VERBATIM([HEXCHATSHAREDIR],[#undef HEXCHATSHAREDIR])
AH_VERBATIM([SOCKS],[#undef SOCKS])
AH_VERBATIM([USE_MSPROXY],[#undef USE_MSPROXY])
AH_VERBATIM([USE_LIBPROXY],[#undef USE_LIBPROXY])
dnl AH_VERBATIM([USE_GNOME],[#undef USE_GNOME])
AH_VERBATIM([USE_SHM],[#undef USE_SHM])
AH_VERBATIM([USE_GTKSPELL],[#undef USE_GTKSPELL])
AH_VERBATIM([USE_LIBSEXY],[#undef USE_LIBSEXY])
AH_VERBATIM([HAVE_ISO_CODES],[#undef HAVE_ISO_CODES])
AH_VERBATIM([USE_LIBNOTIFY],[#undef USE_LIBNOTIFY])
AH_VERBATIM([USE_LIBCANBERRA],[#undef USE_LIBCANBERRA])
AH_VERBATIM([USE_IPV6],[#undef USE_IPV6])
AH_VERBATIM([USE_MMX],[#undef USE_MMX])
AH_VERBATIM([USE_OPENSSL],[#undef USE_OPENSSL])
AH_VERBATIM([USE_PLUGIN],[#undef USE_PLUGIN])
AH_VERBATIM([USE_XFT],[#undef USE_XFT])
AH_VERBATIM([USE_XLIB],[#undef USE_XLIB])
AH_VERBATIM([USE_SIGACTION],[#undef USE_SIGACTION])
AH_VERBATIM([USING_FREEBSD],[#undef USING_FREEBSD])
AH_VERBATIM([USING_LINUX],[#undef USING_LINUX])
AH_VERBATIM([socklen_t],[#undef socklen_t])
AH_VERBATIM([USE_DBUS],[#undef USE_DBUS])

AC_PATH_PROG(sedpath, sed)
if test "_$sedpath" = _; then
	AC_MSG_ERROR(Cannot find sed: I need it!)
fi

AC_PATH_PROG(unamepath, uname)
if test "_$unamepath" = _; then
	system="unknown"
else
	AC_MSG_CHECKING(system type)
	system=`$unamepath -s`
	AC_MSG_RESULT($system)
	if test "$system" = "Linux"; then
		AC_DEFINE(USING_LINUX)
	fi
	if test "$system" = "FreeBSD"; then
		AC_DEFINE(USING_FREEBSD)
	fi
fi

dnl *********************************************************************
dnl ** configure switches ***********************************************
dnl *********************************************************************

AC_ARG_ENABLE(socks,
	[AS_HELP_STRING([--enable-socks],[link with SOCKS5 library (default: no)])],
        socks=$enableval, socks=no)

AC_ARG_ENABLE(ipv6,
	[AS_HELP_STRING([--disable-ipv6],[disable IPv6])],
        ipv6=$enableval, ipv6=yes)

AC_ARG_ENABLE(xft,
	[AS_HELP_STRING([--enable-xft],[enable use of Xft directly (default: no)])],
        xft=$enableval, xft=no)

AC_ARG_ENABLE(openssl,
	[AS_HELP_STRING([--enable-openssl[=PATH]],[enable use of openSSL])],
        openssl=$enableval, openssl=yes)

AC_ARG_ENABLE(gtkfe,
	[AS_HELP_STRING([--disable-gtkfe],[disable building gtk frontend])],
        gtkfe=$enableval, gtkfe=yes)

AC_ARG_ENABLE(textfe,
	[AS_HELP_STRING([--enable-textfe],[build the text frontend (default: no)])],
        textfe=$enableval, textfe=no)

dnl AC_ARG_ENABLE(gnome,
dnl 	[AS_HELP_STRING([--disable-gnome],[disable use of gnome])],
dnl        gnome=$enableval, gnome=yes)

AC_ARG_ENABLE(xlib,
	[AS_HELP_STRING([--disable-xlib],[disable use of xlib (for non X11 systems)])],
        xlib=$enableval, xlib=yes)

AC_ARG_ENABLE(python,
	[AS_HELP_STRING([--enable-python=pythonversion],[build the python plugin (default on, python2)])],
        python=$enableval, python=python2)

AC_ARG_ENABLE(perl,
	[AS_HELP_STRING([--disable-perl],[don\'t build the perl plugin])],
        perl=$enableval, perl=yes)

AC_ARG_ENABLE(perl_old,
	[AS_HELP_STRING([--disable-perl_old],[no backwards compatibility for perl plugin])],
        perl_old=$enableval, perl_old=yes)

AC_ARG_ENABLE(plugin,
	[AS_HELP_STRING([--disable-plugin],[disable plugin support])],
        plugin=$enableval, plugin=yes)

AC_ARG_ENABLE(checksum,
	[AS_HELP_STRING([--disable-checksum],[disable the Checksum plugin])],
        checksum=$enableval, checksum=yes)

AC_ARG_ENABLE(doat,
	[AS_HELP_STRING([--disable-doat],[disable the Do At plugin])],
        doat=$enableval, doat=yes)

AC_ARG_ENABLE(fishlim,
	[AS_HELP_STRING([--disable-fishlim],[disable the FiSHLiM plugin])],
        fishlim=$enableval, fishlim=yes)

AC_ARG_ENABLE(sysinfo,
	[AS_HELP_STRING([--disable-sysinfo],[disable the SysInfo plugin])],
        sysinfo=$enableval, sysinfo=yes)

AC_ARG_ENABLE(dbus,
	[AS_HELP_STRING([--disable-dbus],[disable DBUS support])],
        dbus=$enableval, dbus=yes)

AC_ARG_ENABLE(libnotify,
	[AS_HELP_STRING([--disable-libnotify],[disable libnotify support])],
        libnotify=$enableval, libnotify=yes)

AC_ARG_ENABLE(libcanberra,
	[AS_HELP_STRING([--disable-libcanberra],[disable libcanberra support])],
		libcanberra=$enableval, libcanberra=yes)


AC_ARG_ENABLE(mmx,
	[AS_HELP_STRING([--disable-mmx],[disable MMX assembly routines])],
        mmx=$enableval, mmx=yes)

AC_ARG_ENABLE(shm,
	[AS_HELP_STRING([--enable-shm],[enable use of XShm for fast tinting (default: no)])],
        shm=$enableval, shm=no)

AC_ARG_ENABLE(spell,
	[AS_HELP_STRING([--enable-spell=type],[enable spelling type: none static libsexy gtkspell])],
		  spell=$enableval, spell=libsexy)

AC_ARG_ENABLE(ntlm,
	[AS_HELP_STRING([--enable-ntlm],[enable Microsoft\'s NTLM auth (libntlm) library support (default: no)])],
	ntlm=$enableval, ntlm=no)

AC_ARG_ENABLE(libproxy,
	[AS_HELP_STRING([--disable-libproxy],[disable libproxy support (default: auto)])],
        libproxy=$enableval, libproxy=auto)


dnl *********************************************************************
dnl ** GLIB *************************************************************
dnl *********************************************************************

AM_PATH_GLIB_2_0(2.28.0, glib=yes, glib=no)
if test "$glib" = no; then
	AC_MSG_ERROR(Cannot find GLib!)
fi

PKG_CHECK_MODULES([GOBJECT], [gobject-2.0], [], [AC_MSG_ERROR(Cannot find gobject-2.0!)])

COMMON_CFLAGS="$GLIB_CFLAGS $GOBJECT_CFLAGS -DG_DISABLE_SINGLE_INCLUDES"
COMMON_LIBS="$GLIB_LIBS $GOBJECT_LIBS -lgmodule-2.0"

dnl *********************************************************************
dnl ** GTK **************************************************************
dnl *********************************************************************

# we might get undefined macro without this test
if test "$gtkfe" = yes ; then
	AM_PATH_GTK_2_0(2.24.0, havegtk=yes, havegtk=no)

	if test "$havegtk" = no; then
		gtkfe=no
		echo
		echo Cannot find GTK\! Not building GTK FrontEnd.
		echo
	fi
fi

if test "$gtkfe" != yes; then
	gnome=no
fi

dnl *********************************************************************
dnl ** GNOME ************************************************************
dnl *********************************************************************

GUI_LIBS="$GUI_LIBS $GTK_LIBS"
GUI_CFLAGS="$GUI_CFLAGS $GTK_CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"

#if test "$gnome" = yes; then
#	AC_PATH_PROG(pkgconfigpath, pkg-config)
#	AC_MSG_CHECKING(Gnome2 compile flags)
#	GNOME_CFLAGS="`$pkgconfigpath libgnome-2.0 --cflags 2>/dev/null`"
#	if test "_$GNOME_CFLAGS" = _ ; then
#		gnome=no
#		AC_MSG_RESULT([Gnome not found, building without it.])
#	else
#		GNOME_VER="`$pkgconfigpath libgnome-2.0 --modversion`"
#		GUI_LIBS="$GUI_LIBS `$pkgconfigpath libgnome-2.0 --libs`"
#		GUI_CFLAGS="$GUI_CFLAGS $GNOME_CFLAGS"
#		AC_DEFINE(USE_GNOME)
#		AC_MSG_RESULT(ok)
#	fi
#fi

# GConf
#if test "$gnome" != no ; then
#	AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
#else
#	GCONFTOOL=no
#fi

dnl *********************************************************************
dnl ** XFT **************************************************************
dnl *********************************************************************

if test "$xft" = yes; then
	AC_PATH_PROG(pkgconfigpath, pkg-config)
	if $pkgconfigpath xft --exists; then
		GUI_CFLAGS="$GUI_CFLAGS `$pkgconfigpath xft --cflags`"
		GUI_LIBS="$GUI_LIBS `$pkgconfigpath xft --libs`"
	else
		xft=no
		oldCPPFLAGS=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"
		AC_CHECK_HEADERS(X11/Xft/Xft.h, xft=yes)
		CPPFLAGS=$oldCPPFLAGS
	fi
	if test "$xft" = yes; then
		AC_DEFINE(USE_XFT)
	fi
fi

dnl *********************************************************************
dnl ** XLIB *************************************************************
dnl *********************************************************************

if test "$xlib" = yes; then
	AC_DEFINE(USE_XLIB)
	if test "$system" = "SunOS"; then
		LIBS="$LIBS -L/usr/openwin/lib -lX11"
	else
		AC_CHECK_LIB(X11, XSetWMHints)
	fi
else
	shm=no
fi

dnl *********************************************************************
dnl ** PERL *************************************************************
dnl *********************************************************************

if test "$perl" = yes; then
	AC_PATH_PROG(perlpath, perl)
	AC_MSG_CHECKING(for Perl compile flags)
	PERL_CFLAGS=`$perlpath -MExtUtils::Embed -e ccopts 2>/dev/null`
	if test "_$PERL_CFLAGS" = _ ; then
		AC_MSG_RESULT([not found, building without perl.])
		perl=no
	else
		PERL_LDFLAGS=`$perlpath -MExtUtils::Embed -e ldopts |$sedpath 's/-lgdbm //'`
		PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-ldb //'`
		PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lndbm //'`
		if test "$system" = "Linux"; then
			PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lnsl //'`
			PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lposix //'`
		fi
		PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lc //'`
		AC_MSG_RESULT(ok)

#		oldLIBS=$LIBS
#		LIBS="$LIBS $PERL_LDFLAGS"
#		AC_CHECK_FUNC(eval_pv)
#		AC_CHECK_FUNC(call_pv)
#		LIBS=$oldLIBS

		AC_MSG_CHECKING(for perl >= 5.8.0)
		PERL_VER=`$perlpath -e 'print $]>= 5.008?"yes":"no"'`
		if test "$PERL_VER" = "yes"; then
			AC_MSG_RESULT(yes)
			AC_MSG_CHECKING(if perl plugin will be backward compatible)
			if test "$perl_old" = "yes"; then
					  AC_MSG_RESULT(yes)
					  AC_DEFINE(OLD_PERL)
			else
					  AC_MSG_RESULT(no)
			fi
		else
			AC_MSG_RESULT(no)
			echo "perl version too old, building without perl."
			perl=no
		fi
	fi
fi

dnl *********************************************************************
dnl ** PYTHON ***********************************************************
dnl *********************************************************************

if test "x$python" != xno ; then
	if test "$python" = yes ; then
		# default
		python="python2"
	fi
	AC_PATH_PROG(pythonpath, $python)
	if test "_$pythonpath" = _ ; then
                AC_PATH_PROG(pythonpath, python)
        fi
        if test "_$pythonpath" = _ ; then
                python=no
        else
                AC_MSG_CHECKING(Python version)
                changequote(<<, >>)dnl
                PY_VER=`$pythonpath -c 'import distutils.sysconfig; print(distutils.sysconfig.get_config_vars("VERSION")[0]);'`
                PY_LIB=`$pythonpath -c 'import distutils.sysconfig; print(distutils.sysconfig.get_python_lib(standard_lib=1));'`
                PY_INC=`$pythonpath -c 'import distutils.sysconfig; print(distutils.sysconfig.get_python_inc());'`
                PYPLAT_INC=`$pythonpath -c 'import distutils.sysconfig; print(distutils.sysconfig.get_python_inc(plat_specific=True));'`
                changequote([, ])dnl
                AC_MSG_RESULT($PY_VER)
                $pythonpath -c "import sys; float(sys.version[[:3]]) >= 2.6 and int(sys.version[[0]]) <= 3 or sys.exit(1)"
                if test "$?" != "1"; then
                        AC_MSG_CHECKING(Python compile flags)
                        PY_PREFIX=`$pythonpath -c 'import sys; print(sys.prefix)'`
                        PY_EXEC_PREFIX=`$pythonpath -c 'import sys; print(sys.exec_prefix)'`
                        if test -f $PY_INC/Python.h || test -f $PYPLAT_INC/Python.h; then
								AS_VERSION_COMPARE($PY_VER, 3.0,
									[PYL="$PY_VER"], # less than
									[PYL="${PY_VER}m"], # equal
									[PYL="${PY_VER}m"]) # greater than
                                PY_LIBS="-L$PY_LIB/config -lpython$PYL -lpthread -lutil"
                                PY_CFLAGS="-I$PY_INC -I$PYPLAT_INC"
                                AC_MSG_RESULT(ok)
                        else
                                python=no
                                AC_MSG_RESULT([Can\'t find Python.h])
                        fi
                else
                        echo "Python is too old or too new. Only 2.6-3.x are supported."
                        python=no
                fi
        fi
fi

dnl *********************************************************************
dnl ** IPv6 *************************************************************
dnl *********************************************************************

dnl purely for Solaris
AC_CHECK_FUNC(select, ,
	AC_CHECK_LIB(socket, select, ,
		AC_CHECK_LIB(nsl, select, ,
			AC_CHECK_LIB(inet, select, ,
				AC_CHECK_LIB(cposix, select, ,
					AC_CHECK_LIB(net, select, ,
						AC_MSG_WARN(i can not find select.  you might need to help me)))))))
AC_CHECK_LIB(socket, select)

if test "$ipv6" = yes; then
	AC_CHECK_FUNCS(getaddrinfo, have_getaddrinfo=yes)
	AC_MSG_CHECKING(whether to enable IPv6 support)
	if test "$have_getaddrinfo" = yes; then
		AC_MSG_RESULT(yes)
		AC_DEFINE(USE_IPV6)
	else
		ipv6=no
		AC_MSG_RESULT(no)
	fi
fi

dnl *********************************************************************
dnl ** OPENSSL **********************************************************
dnl *********************************************************************

retry=no
if test "$openssl" != no; then
	AC_PATH_PROG(pkgconfigpath, pkg-config)
	AC_MSG_CHECKING(for openssl through pkg-config)
	if $pkgconfigpath openssl --exists; then
		CPPFLAGS="$CPPFLAGS `$pkgconfigpath openssl --cflags`"
		LIBS="$LIBS `$pkgconfigpath openssl --libs`"
		AC_DEFINE(USE_OPENSSL)
		AC_MSG_RESULT(yes)
		openssl=yes
	else
		AC_MSG_RESULT(no)
		retry=yes
	fi
fi

if test "$retry" = "yes"; then
	unset openssl_path ac_cv_lib_ssl_SSL_new ac_cv_header_openssl_ssl_h
	if test "$openssl" != yes; then
		openssl_path=$openssl
	fi
	openssl=no
	SAVED_LIBS=$LIBS
	LIBS="$LIBS -lcrypto"
	if test -n "$openssl_path"; then
		LIBS="-L$openssl_path/lib $LIBS"
	fi
	AC_CHECK_LIB(ssl, SSL_new, have_openssl=yes)
	LIBS=$SAVED_LIBS
	if test "$have_openssl" = yes; then
		SAVED_CPPFLAGS=$CPPFLAGS
		if test -n "$openssl_path"; then
			CPPFLAGS="-I$openssl_path/include $CPPFLAGS"
		fi
		AC_CHECK_HEADERS(openssl/ssl.h, have_openssl_h=yes)
		if test "$have_openssl_h" = yes; then
			openssl=yes
			AC_DEFINE(USE_OPENSSL)
			LIBS="$LIBS -lssl -lcrypto"
			if test -n "$openssl_path"; then
				LIBS="-L$openssl_path/lib $LIBS"
			fi
		else
			CPPFLAGS=$SAVED_CPPFLAGS
		fi
	fi
fi

dnl *********************************************************************
dnl ** LIBPROXY *********************************************************
dnl *********************************************************************

if test "x$libproxy" = "xyes" -o "x$libproxy" = "xauto" ; then
	PKG_CHECK_MODULES([LIBPROXY], [libproxy-1.0], [
		COMMON_LIBS="$COMMON_LIBS $LIBPROXY_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $LIBPROXY_CFLAGS"
		AC_DEFINE(USE_LIBPROXY)
		libproxy=yes
	], [
		if test "x$libproxy" = "xyes" ; then
			AC_MSG_ERROR(Cannot find libproxy!)
		fi
		libproxy=no
	])
else
	libproxy=no
fi

dnl *********************************************************************
dnl ** PLUGIN ***********************************************************
dnl *********************************************************************

if test "$plugin" = yes; then
	AC_CHECK_FUNCS(dlopen, have_dl=yes)
	if test "$have_dl" != yes; then
		AC_CHECK_LIB(dl, dlopen, have_dl=yes)
		if test "$have_dl" = yes; then
			LIBS="$LIBS -ldl"
		fi
	fi
	if test "$have_dl" = yes; then
		AC_DEFINE(USE_PLUGIN)
		AC_PATH_PROG(pkgconfigpath, pkg-config)
		dnl we just need the -Wl,--export-dynamic, but not -lgmodule-2.0
		RDYNAMIC_FLAGS="`$pkgconfigpath gmodule-2.0 --libs | $sedpath 's/ -lgmodule-2.0//'`"
		LIBS="$LIBS $RDYNAMIC_FLAGS"
		if test "$LD" = ""; then
			VS="`ld --help | grep version-script 2> /dev/null`"
		else
			VS="`$LD --help | grep version-script 2> /dev/null`"
		fi
		if test "$VS" != ""; then
			GUI_LIBS="$GUI_LIBS -Wl,--version-script,\$(srcdir)/../version-script"
		fi
	else
		plugin=no
	fi
fi

dnl *********************************************************************
dnl ** Checksum *********************************************************
dnl *********************************************************************

if test "$checksum" != "no"; then
	checksum=no
	AC_MSG_CHECKING(for plugin interface used by Checksum)
	if test "$plugin" = yes; then
		AC_MSG_RESULT([yes])
		AC_MSG_CHECKING(for OpenSSL used by Checksum)
		if test "$openssl" = yes; then
			checksum=yes
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([OpenSSL cannot be found, use the --enable-openssl option])
		fi
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option])
	fi
fi

dnl *********************************************************************
dnl ** DO AT ************************************************************
dnl *********************************************************************

if test "$doat" != "no"; then
	AC_MSG_CHECKING(for plugin interface used by Do At)
	doat=no
	if test "$plugin" = yes; then
		doat=yes
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for Do At])
	fi
fi

dnl *********************************************************************
dnl ** FiSHLiM **********************************************************
dnl *********************************************************************

if test "$fishlim" != "no"; then
	fishlim=no
	AC_MSG_CHECKING(for plugin interface used by FiSHLiM)
	if test "$plugin" = yes; then
		AC_MSG_RESULT([yes])
		AC_MSG_CHECKING(for OpenSSL used by FiSHLiM)
		if test "$openssl" = yes; then
			fishlim=yes
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([OpenSSL cannot be found, use the --enable-openssl option])
		fi
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option])
	fi
fi

dnl *********************************************************************
dnl ** SYSINFO **********************************************************
dnl *********************************************************************

if test "$sysinfo" != "no"; then
	AC_MSG_CHECKING(for plugin interface used by SysInfo)
	if test "$plugin" = yes; then
		AC_MSG_RESULT([yes])
		PKG_CHECK_MODULES(LIBPCI, libpci >= 3.0.0, [sysinfo=yes], [sysinfo=no])
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for SysInfo])
		sysinfo=no
	fi
fi

dnl #######################################################################
dnl # Check for DBUS libraries
dnl #######################################################################

if test "x$dbus" = "xyes" ; then
	PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.60 dbus-glib-1 >= 0.60 gthread-2.0], dbus=yes, [
		dbus=no
	])
	AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool, no)
	AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
	if test "x$DBUS_BINDING_TOOL" = "xno" || test "x$GLIB_GENMARSHAL" = "xno" || test "x$dbus" = "xno" ; then
		dbus="no"
	else
		COMMON_LIBS="$COMMON_LIBS $DBUS_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $DBUS_CFLAGS"
		AC_DEFINE(USE_DBUS)

		AS_AC_EXPAND(DBUS_SERVICES_DIR, "$datadir/dbus-1/services")
		AC_SUBST(DBUS_SERVICES_DIR)
		AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBUS is])
	fi
fi

dnl *********************************************************************
dnl ** LIBNOTIFY ********************************************************
dnl *********************************************************************

if test "x$libnotify" = "xyes" ; then
	PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.4, [], [
		libnotify=no
	])
	if test "$libnotify" != "no" ; then
		GUI_LIBS="$GUI_LIBS $LIBNOTIFY_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $LIBNOTIFY_CFLAGS"
		AC_DEFINE(USE_LIBNOTIFY)
	fi
fi

dnl *********************************************************************
dnl ** LIBCANBERRA ******************************************************
dnl *********************************************************************

if test "x$libcanberra" = "xyes" ; then
	PKG_CHECK_MODULES(LIBCANBERRA, libcanberra >= 0.22, [], [
		libcanberra=no
	])
	if test "$libcanberra" != "no" ; then
		COMMON_LIBS="$COMMON_LIBS $LIBCANBERRA_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $LIBCANBERRA_CFLAGS"
		AC_DEFINE(USE_LIBCANBERRA)
	fi
fi

dnl *********************************************************************
dnl ** SPELL ************************************************************
dnl *********************************************************************

if test "$spell" = "gtkspell" ; then
	PKG_CHECK_MODULES(GTKSPELL, gtkspell-2.0 >= 2.0.2, [], [
		spell=no
	])
	if test "$spell" != "no" ; then
		GUI_LIBS="$GUI_LIBS $GTKSPELL_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $GTKSPELL_CFLAGS"
		AC_DEFINE(USE_GTKSPELL)
	fi
fi

if test "$spell" = "libsexy" ; then
	PKG_CHECK_MODULES([LIBSEXY], [libsexy >= 0.1.8], [
		libsexy=yes
		GUI_LIBS="$GUI_LIBS $LIBSEXY_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $LIBSEXY_CFLAGS"
		AC_DEFINE(USE_LIBSEXY)
	], [
		dnl use builtin static one
		spell="static"
	])
fi

if test "$spell" = "static" ; then
	PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= 2.0.0, [
	AC_DEFINE(HAVE_ISO_CODES)
	AC_DEFINE(USE_LIBSEXY)
	GUI_CFLAGS="$GUI_CFLAGS $LIBXML2_CFLAGS"
	LIBS="$LIBS -lxml2"
	], [
	AC_MSG_ERROR(Cannot find libxml2!)
	])
fi

dnl *********************************************************************
dnl ** CONDITIONALS *****************************************************
dnl *********************************************************************

AM_CONDITIONAL(USE_OPENSSL, test "x$openssl" = "xyes")
AM_CONDITIONAL(USE_LIBSEXY, test "x$spell" = "xstatic")
AM_CONDITIONAL(USE_LIBNOTIFY, test "x$libnotify" = "xyes")
AM_CONDITIONAL(USE_LIBCANBERRA, test "x$libcanberra" = "xyes")
AM_CONDITIONAL(DO_TEXT, test "x$textfe" = "xyes")
AM_CONDITIONAL(DO_GTK, test "x$gtkfe" = "xyes")
AM_CONDITIONAL(DO_PERL, test "x$perl" = "xyes")
AM_CONDITIONAL(DO_PYTHON, test "x$python" != "xno")
AM_CONDITIONAL(DO_PLUGIN, test "x$plugin" = "xyes")
AM_CONDITIONAL(DO_CHECKSUM, test "x$checksum" = "xyes")
AM_CONDITIONAL(DO_DOAT, test "x$doat" = "xyes")
AM_CONDITIONAL(DO_FISHLIM, test "x$fishlim" = "xyes")
AM_CONDITIONAL(DO_SYSINFO, test "x$sysinfo" = "xyes")
AM_CONDITIONAL(USE_DBUS, test "x$dbus" = "xyes")
#AM_CONDITIONAL(DO_GCONF, test "x$GCONFTOOL" != "xno")

dnl *********************************************************************
dnl ** SOCKS5 ***********************************************************
dnl *********************************************************************

if test "$socks" = yes; then
	socks=no
	AC_CHECK_LIB(socks5, SOCKSconnect, have_socks=yes)
	if test "$have_socks" = yes; then
		AC_CHECK_HEADERS(socks.h, have_socks_h=yes)
		if test "$have_socks_h" = yes; then
			socks=yes
			AC_DEFINE(SOCKS)
			LIBS="$LIBS -lsocks5"
		fi
	fi
fi

dnl *********************************************************************
dnl ** MS PROXY *********************************************************
dnl *********************************************************************

have_ntlm="no"
if test "x$ntlm" = "xyes" ; then
	have_ntlm="no"
	AC_CHECK_LIB(ntlm, ntlm_smb_encrypt, have_ntlm=yes)
	if test "$have_ntlm" = yes; then
		LIBS="$LIBS -lntlm"
		AC_DEFINE(USE_MSPROXY)
	fi
fi

dnl *********************************************************************
dnl ** XShm *************************************************************
dnl *********************************************************************

if test "$shm" = yes; then
	oldl=$LIBS
	oldc=$CPPFLAGS
	LIBS="$LIBS `$pkgconfigpath --libs-only-L xft`"
	CPPFLAGS="$CPPFLAGS `$pkgconfigpath --cflags-only-I xft`"
	shm=no
	AC_CHECK_LIB(Xext, XShmAttach, shm=yes)
	if test "$shm" = yes; then
		shm=no
		AC_CHECK_HEADERS(sys/ipc.h, shm=yes)
		if test "$shm" = yes; then
			shm=no
			AC_CHECK_HEADERS(sys/shm.h, shm=yes)
		fi
	fi

	LIBS=$oldl
	if test "$shm" = yes; then
		GUI_LIBS="$GUI_LIBS `$pkgconfigpath --libs-only-L xft` -lX11 -lXext"
		AC_DEFINE(USE_SHM)
	else
		CPPFLAGS=$oldc
	fi
fi

dnl *********************************************************************
dnl ** MMX **************************************************************
dnl *********************************************************************

dnl we don't need mmx on *this* machine, just i386, because
dnl it's checked at runtime.
if test "$mmx" = "yes"; then
	case $host_cpu in
    i386|i486|i586|i686|i786|k6|k7)
    mmx=yes
        ;;
     *)
    mmx=no
	esac
	if test "$system" = "OpenBSD"; then
		dnl openbsd fails because mmx_cmod doesn't prefix its symbols with underscore.
		dnl xtext.o: Undefined symbol `_shade_ximage_15_mmx' referenced from text segment
		mmx=no
	fi
	if test "$mmx" = "yes"; then
		AC_DEFINE(USE_MMX)
	fi
fi

AM_CONDITIONAL(USE_MMX, test "$mmx" = "yes")

dnl *********************************************************************
dnl ** GCC FLAGS ********************************************************
dnl *********************************************************************

dnl Only use -Wall and -pipe if we have gcc
if test "x$GCC" = "xyes"; then
	if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
		CFLAGS="$CFLAGS -Wall"
	fi
	if test "$system" = "Linux" -o "$system" = "FreeBSD"; then
		if test -z "`echo "$CFLAGS" | grep "\-pipe" 2> /dev/null`" ; then
			CFLAGS="$CFLAGS -pipe"
		fi
	fi
	if test -z "`echo "$CFLAGS" | grep "\-g " 2> /dev/null`" ; then
		CFLAGS="$CFLAGS -g"
	fi
fi

dnl does this compiler support -Wno-pointer-sign ?
AC_MSG_CHECKING([if gcc accepts -Wno-pointer-sign ])

safe_CFLAGS=$CFLAGS
CFLAGS="-Wno-pointer-sign"

AC_TRY_COMPILE(, [
int main () { return 0 ; }
],
[
no_pointer_sign=yes
AC_MSG_RESULT([yes])
], [
no_pointer_sign=no
AC_MSG_RESULT([no])
])
CFLAGS=$safe_CFLAGS

if test x$no_pointer_sign = xyes; then
	CFLAGS="$CFLAGS -Wno-pointer-sign"
fi

dnl does this compiler support -funsigned-char ?
AC_MSG_CHECKING([if gcc accepts -funsigned-char ])

safe_CFLAGS=$CFLAGS
CFLAGS="-funsigned-char"

AC_TRY_COMPILE(, [
int main () { return 0 ; }
],
[
unsigned_char=yes
AC_MSG_RESULT([yes])
], [
unsigned_char=no
AC_MSG_RESULT([no])
])
CFLAGS=$safe_CFLAGS

if test x$unsigned_char = xyes; then
	CFLAGS="$CFLAGS -funsigned-char"
fi

dnl does this compiler support -Wno-unused-result ?
AC_MSG_CHECKING([if gcc accepts -Wno-unused-result ])

safe_CFLAGS=$CFLAGS
CFLAGS="-Wno-unused-result"

AC_TRY_COMPILE(, [
int main () { return 0 ; }
],
[
no_unused_result=yes
AC_MSG_RESULT([yes])
], [
no_unused_result=no
AC_MSG_RESULT([no])
])
CFLAGS=$safe_CFLAGS

if test x$no_unused_result = xyes; then
	CFLAGS="$CFLAGS -Wno-unused-result"
fi

dnl *********************************************************************
dnl ** FUNCTIONS/LIBS/CFLAGS ********************************************
dnl *********************************************************************

AC_MSG_CHECKING(for modern sigaction)
dnl libc5 on linux and FreeBSD 3.x doesn\'t have siginfo_t
dnl and the sa_sigation field.
AC_TRY_COMPILE(
	[#include <signal.h>],
	[struct sigaction act;
	 siginfo_t *si;
	 act.sa_sigaction = 0;],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE(USE_SIGACTION) 
	],
	AC_MSG_RESULT(no))

AC_PATH_PROG(gdkpixbufcsourcepath, gdk-pixbuf-csource)
AC_SUBST(gdkpixbufcsourcepath)
if test "$gtkfe" != no -a "_$gdkpixbufcsourcepath" = _; then
	AC_MSG_ERROR(Cannot find gdk-pixbuf-csource: Install GTK+ 2.0!)
fi

dnl if we don\'t have this, use g_snprintf instead
AC_CHECK_FUNCS(snprintf vsnprintf memrchr strtoull)

AC_CHECK_FUNC(gethostbyname, ,
	AC_CHECK_LIB(resolv, gethostbyname, ,
		AC_CHECK_LIB(nsl, gethostbyname)))

AC_CHECK_FUNC(gethostname, , AC_CHECK_LIB(nsl, gethostname))

dnl necessary for IRIX
AC_CHECK_HEADERS(strings.h)

dnl Check for type in sys/socket.h - from Squid source (GPL)
AC_CACHE_CHECK(for socklen_t, ac_cv_type_socklen_t, [
AC_EGREP_CPP([socklen_t[^a-zA-Z_0-9]], [#include <sys/types.h>
#include <sys/socket.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif],
ac_cv_type_socklen_t=yes,
ac_cv_type_socklen_t=no)
])
if test $ac_cv_type_socklen_t = no; then
	AC_DEFINE(socklen_t, int)
fi

dnl Mac OS X and Darwin use lookupd, which caches DNS queries by default
AC_EGREP_CPP(lookupd, dnl
[#if (defined(__APPLE__) && defined(__MACH__))
	lookupd
#endif], AC_DEFINE([LOOKUPD],1,[Define to 1 if the system uses lookupd]))

dnl freebsd needs this
LIBS="$LIBS $INTLLIBS"
CFLAGS="$CFLAGS $CPPFLAGS"

GUI_LIBS="$GUI_LIBS $COMMON_LIBS"

dnl make these visible to all Makefiles
AC_SUBST(GUI_LIBS)
AC_SUBST(GUI_CFLAGS)
AC_SUBST(COMMON_LIBS)
AC_SUBST(COMMON_CFLAGS)
AC_SUBST(PERL_CFLAGS)
AC_SUBST(PERL_LDFLAGS)
AC_SUBST(PY_CFLAGS)
AC_SUBST(PY_LIBS)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR], AC_SUBST([pkgconfigdir], ${libdir}/pkgconfig))

PLUGIN_INCLUDES='-I$(top_srcdir)/plugins'
AC_SUBST(PLUGIN_INCLUDES)

dnl for plugin.c and pixmaps.c
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
test "x$exec_prefix" = xNONE && exec_prefix="$prefix"

AC_DEFINE_UNQUOTED(PREFIX, "${prefix}")

AS_AC_EXPAND(HEXCHATLIBDIR, "${libdir}/hexchat/plugins")
AC_DEFINE_UNQUOTED(HEXCHATLIBDIR, "$HEXCHATLIBDIR")

AS_AC_EXPAND(HEXCHATSHAREDIR, "$datadir")
AC_DEFINE_UNQUOTED(HEXCHATSHAREDIR, "$HEXCHATSHAREDIR")

dnl for plugins/xxx/Makefile.am
hexchatlibdir=${libdir}/hexchat/plugins
AC_SUBST(hexchatlibdir)

AC_CONFIG_FILES([
Makefile
share/Makefile
share/icons/Makefile
share/misc/Makefile
share/pkgconfig/Makefile
share/pkgconfig/hexchat-plugin.pc
src/Makefile
src/common/Makefile
src/common/dbus/Makefile
src/fe-text/Makefile
src/fe-gtk/Makefile
src/pixmaps/Makefile
plugins/Makefile
plugins/python/Makefile
plugins/perl/Makefile
plugins/checksum/Makefile
plugins/doat/Makefile
plugins/fishlim/Makefile
plugins/sysinfo/Makefile
intl/Makefile
po/Makefile.in
man/Makefile
])

AC_OUTPUT

echo
echo HexChat $VERSION
echo
echo GTK+ interface ........ : $gtkfe
echo Text interface ........ : $textfe
echo
echo MMX tinting ........... : $mmx
echo XShm tinting .......... : $shm
if test "$xft" = no; then
  echo Text backend .......... : Pango
else
  echo Text backend .......... : Xft
fi
echo OpenSSL support ....... : $openssl
echo D-Bus support ......... : $dbus
echo libnotify support ..... : $libnotify
echo libcanberra support ... : $libcanberra
echo Spelling .............. : $spell
echo Plugin interface ...... : $plugin
echo NLS/gettext ........... : $USE_NLS
echo IPv6 support .......... : $ipv6
echo MS Proxy NTLM \(ISA\) ... : $have_ntlm
echo libproxy support ...... : $libproxy
echo
echo Perl .................. : $perl
echo Python ................ : $python
echo
echo Checksum .............. : $checksum
echo Do At ................. : $doat
echo FiSHLiM ............... : $fishlim
echo SysInfo ............... : $sysinfo
echo
echo The binary will be installed in $prefix/bin
echo

if test "$gtkfe" = no; then
	echo Warning: The GTK \(GUI\) frontend will not be built.
	echo
fi

if test "$spell" = "gtkspell"; then
	echo Warning: GTK SPELL is not the recommended spelling library.
	echo
fi

echo configure complete, now type \'make\' and pray.
echo
