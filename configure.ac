dnl Process this file with autoconf to produce a configure script.

AC_INIT([HexChat],[2.11.0])

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
AM_PROG_AR
AM_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PATH_PROG(MDTOOL, mdtool, no)

dnl -----------------------------------------------------------
dnl Language Support
dnl -----------------------------------------------------------
GETTEXT_PACKAGE=hexchat
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])
IT_PROG_INTLTOOL([0.40.0])
AM_GLIB_GNU_GETTEXT

dnl displaced from acconfig.h
AH_VERBATIM([OLD_PERL],[#undef OLD_PERL])
AH_VERBATIM([PREFIX],[#undef PREFIX])
AH_VERBATIM([HEXCHATLIBDIR],[#undef HEXCHATLIBDIR])
AH_VERBATIM([HEXCHATSHAREDIR],[#undef HEXCHATSHAREDIR])
AH_VERBATIM([SOCKS],[#undef SOCKS])
AH_VERBATIM([USE_MSPROXY],[#undef USE_MSPROXY])
AH_VERBATIM([USE_LIBPROXY],[#undef USE_LIBPROXY])
AH_VERBATIM([HAVE_ISO_CODES],[#undef HAVE_ISO_CODES])
AH_VERBATIM([HAVE_GTK_MAC],[#undef HAVE_GTK_MAC])
AH_VERBATIM([USE_LIBNOTIFY],[#undef USE_LIBNOTIFY])
AH_VERBATIM([USE_LIBCANBERRA],[#undef USE_LIBCANBERRA])
AH_VERBATIM([USE_IPV6],[#undef USE_IPV6])
AH_VERBATIM([USE_OPENSSL],[#undef USE_OPENSSL])
AH_VERBATIM([USE_PLUGIN],[#undef USE_PLUGIN])
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

AC_ARG_ENABLE(openssl,
	[AS_HELP_STRING([--enable-openssl[=PATH]],[enable use of openSSL])],
        openssl=$enableval, openssl=yes)

AC_ARG_ENABLE(gtkfe,
	[AS_HELP_STRING([--disable-gtkfe],[disable building gtk frontend])],
        gtkfe=$enableval, gtkfe=yes)

AC_ARG_ENABLE(textfe,
	[AS_HELP_STRING([--enable-textfe],[build the text frontend (default: no)])],
        textfe=$enableval, textfe=no)

AC_ARG_ENABLE(python,
	[AS_HELP_STRING([--enable-python=pythonversion],[build the python plugin; possible values: "python2", "python3" or specific such as "python3.3" (default on, python2)])],
        python=$enableval, python=python2)
AS_IF([test "x$python" = "xyes"], [python=python2])

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

AC_ARG_ENABLE(ntlm,
	[AS_HELP_STRING([--enable-ntlm],[enable Microsoft\'s NTLM auth (libntlm) library support (default: no)])],
	ntlm=$enableval, ntlm=no)

AC_ARG_ENABLE(libproxy,
	[AS_HELP_STRING([--disable-libproxy],[disable libproxy support (default: auto)])],
        libproxy=$enableval, libproxy=auto)

AC_ARG_ENABLE(isocodes,
	[AS_HELP_STRING([--disable-isocodes],[disable iso-codes with spell-check])],
        isocodes=$enableval, isocodes=yes)

AC_ARG_ENABLE(minimal-flags,
	[AS_HELP_STRING([--enable-minimal-flags],[only add those CFLAGS that are really needed or not intrusive (default: no)])],
        minimalflags=$enableval, minimalflags=no)

AC_ARG_ENABLE(static-analysis,
	[AS_HELP_STRING([--enable-static-analysis],[if using clang run static analysis during build (default: no)])],
        analyze=$enableval, analyze=no)

AC_ARG_WITH(theme-manager,
	[AS_HELP_STRING([--with-theme-manager],[compile theme manager (needs monodevelop, default: off)])],
		theme_manager=$withval, theme_manager=no)



dnl *********************************************************************
dnl ** THEME-MANAGER ****************************************************
dnl *********************************************************************
if test "x$theme_manager" != "xno" ; then
	if test "x$MDTOOL" = "xno"; then
		AC_MSG_ERROR([No "mdtool" found, you need to install monodevelop!])
	fi
fi


dnl *********************************************************************
dnl ** GLIB *************************************************************
dnl *********************************************************************

AM_PATH_GLIB_2_0(2.28.0, glib=yes, glib=no)
if test "$glib" = no; then
	AC_MSG_ERROR(Cannot find GLib!)
fi

PKG_CHECK_MODULES([GOBJECT], [gobject-2.0], [], [AC_MSG_ERROR(Cannot find gobject-2.0!)])
PKG_CHECK_MODULES([GIO], [gio-2.0], [], [AC_MSG_ERROR(Cannot find gio-2.0!)])
PKG_CHECK_MODULES([GMODULE], [gmodule-2.0], [], [AC_MSG_ERROR(Cannot find gmodule-2.0!)])

COMMON_CFLAGS="$GLIB_CFLAGS $GIO_CFLAGS $GOBJECT_CFLAGS $GMODULE_CFLAGS -DG_DISABLE_SINGLE_INCLUDES"
COMMON_LIBS="$GLIB_LIBS $GIO_LIBS $GOBJECT_LIBS $GMODULE_LIBS"

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

GUI_LIBS="$GUI_LIBS $GTK_LIBS"
GUI_CFLAGS="$GUI_CFLAGS $GTK_CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED"

dnl *********************************************************************
dnl ** MAC_INTEGRATION **************************************************
dnl *********************************************************************

_gdk_tgt=`$PKG_CONFIG --variable=target gdk-2.0`
if test "x$_gdk_tgt" = xquartz; then
		PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration-gtk2, [
			GUI_LIBS="$GUI_LIBS $GTK_MAC_LIBS"
			GUI_CFLAGS="$GUI_CFLAGS $GTK_MAC_CFLAGS"
			AC_DEFINE(HAVE_GTK_MAC)
		])
fi

dnl *********************************************************************
dnl ** PERL *************************************************************
dnl *********************************************************************

if test "$perl" = yes; then
	AC_MSG_CHECKING(for plugin interface used by Perl)
	if test "$plugin" = yes; then
		AC_MSG_RESULT([yes])
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

			AC_MSG_CHECKING(for perl >= 5.8.0)
			PERL_VER=`$perlpath -e 'print $]>= 5.008?"yes":"no"'`
			if test "$PERL_VER" = "yes"; then
				original_cflags="$CFLAGS"
				original_ldflags="$LDFLAGS"
				CFLAGS="$PERL_CFLAGS"
				LDFLAGS="$PERL_LDFLAGS"
				AC_TRY_LINK([
						#define PERL_NO_INLINE_FUNCTIONS
						#include <EXTERN.h>
						#include <perl.h>
					], [], perl_is_usable=yes, perl_is_usable=no)
				CFLAGS="$original_cflags"
				LDFLAGS="$original_ldflags"
				if test x$perl_is_usable = xno ; then
					AC_MSG_RESULT(no)
					perl=no
				else
					AC_MSG_RESULT(yes)
					AC_MSG_CHECKING(if perl plugin will be backward compatible)
					if test "$perl_old" = "yes"; then
							  AC_MSG_RESULT(yes)
							  AC_DEFINE(OLD_PERL)
					else
							  AC_MSG_RESULT(no)
					fi
				fi
			else
				AC_MSG_RESULT(no)
				echo "perl version too old, building without perl."
				perl=no
			fi
		fi
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for Perl])
		perl=no
	fi
fi

dnl *********************************************************************
dnl ** PYTHON ***********************************************************
dnl *********************************************************************

if test "x$python" != xno ; then
	AC_MSG_CHECKING(for plugin interface used by Python)
	if test "$plugin" = yes; then
		AC_MSG_RESULT([yes])
		case $python in
			dnl set python2 default here
			python2)
				PKG_CHECK_MODULES([PY], [python-2.7],
								  [PY_VER="`$PKG_CONFIG --modversion python-2.7`"],
								  [true])
				;;
			dnl set python3 default here
			python3)
				PKG_CHECK_MODULES([PY], [python-3.4],
								  [PY_VER="`$PKG_CONFIG --modversion python-3.4`"],
								  [true])
				if test "$PY_VER" = "" ; then
					PKG_CHECK_MODULES([PY], [python-3.3],
									  [PY_VER="`$PKG_CONFIG --modversion python-3.3`"],
									  [true])
				fi
				;;
			dnl add broken versions here
			python2.5|python2.6|python3.1|python3.2)
				AC_MSG_WARN(Unsupported Python version ${python}!);;
			python*)
				python="python-${python#python}" # stay posix compliant
				PKG_CHECK_MODULES([PY], [${python}],
								  [PY_VER="`$PKG_CONFIG --modversion ${python}`"],
								  [AC_MSG_WARN(Cannot find "${python}.pc"!)])
				;;
			*)
				AC_MSG_WARN(Unsupported Python ${python}!)
		esac
		AC_MSG_CHECKING(Python version)

		if test "$PY_VER"; then
			AC_MSG_RESULT($PY_VER)
			python="python-${PY_VER}"
		else
			AC_MSG_RESULT(Not found)
			python=no
		fi
	else
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for Python])
		python=no
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
	AC_MSG_CHECKING(for openssl through pkg-config)
	if $PKG_CONFIG openssl --exists; then
		CPPFLAGS="$CPPFLAGS `$PKG_CONFIG openssl --cflags`"
		LIBS="$LIBS `$PKG_CONFIG openssl --libs`"
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
	AC_DEFINE(USE_PLUGIN)
fi

dnl *********************************************************************
dnl ** Checksum *********************************************************
dnl *********************************************************************

if test "$checksum" != "no"; then
	checksum=no
	AC_MSG_CHECKING(for plugin interface used by Checksum)
	if test "$plugin" = yes; then
		checksum=yes
		AC_MSG_RESULT([yes])
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

if test "x$isocodes" = "xyes" ; then
	PKG_CHECK_MODULES(ISOCODES, "iso-codes", [
		iso_codes_prefix=`$PKG_CONFIG --variable=prefix iso-codes 2>/dev/null || echo /usr`
    	AC_MSG_NOTICE([iso-codes prefix: $iso_codes_prefix])
    	AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX], ["$iso_codes_prefix"], [ISO codes prefix])
    	AC_DEFINE_UNQUOTED([ISO_CODES_LOCALEDIR], ["$iso_codes_prefix/share/locale"], [ISO codes locale dir])
    	AC_DEFINE([HAVE_ISO_CODES], [1], [iso-codes available])
    	], [
    	isocodes=no
    	AC_MSG_WARN(iso-codes not found!)
    ])
fi

dnl *********************************************************************
dnl ** Static Analysis **************************************************
dnl *********************************************************************

if test "x$analyze" = "xyes"; then
	if test "$CC" != "clang"; then
		AC_MSG_WARN(CC is not clang for static analysis)
		analyze=no
	fi
fi

dnl *********************************************************************
dnl ** CONDITIONALS *****************************************************
dnl *********************************************************************

AM_CONDITIONAL(USE_OPENSSL, test "x$openssl" = "xyes")
AM_CONDITIONAL(USE_LIBNOTIFY, test "x$libnotify" = "xyes")
AM_CONDITIONAL(USE_LIBCANBERRA, test "x$libcanberra" = "xyes")
AM_CONDITIONAL(USE_MSPROXY, test "x$ntlm" = "xyes")
AM_CONDITIONAL(DO_TEXT, test "x$textfe" = "xyes")
AM_CONDITIONAL(DO_GTK, test "x$gtkfe" = "xyes")
AM_CONDITIONAL(DO_PERL, test "x$perl" = "xyes")
AM_CONDITIONAL(DO_PYTHON, test "x$python" != "xno")
AM_CONDITIONAL(DO_PLUGIN, test "x$plugin" = "xyes")
AM_CONDITIONAL(DO_CHECKSUM, test "x$checksum" = "xyes")
AM_CONDITIONAL(DO_DOAT, test "x$doat" = "xyes")
AM_CONDITIONAL(DO_FISHLIM, test "x$fishlim" = "xyes")
AM_CONDITIONAL(DO_SYSINFO, test "x$sysinfo" = "xyes")
AM_CONDITIONAL(DO_STATIC_ANALYSIS, test "x$analyze" = "xyes")
AM_CONDITIONAL(USE_DBUS, test "x$dbus" = "xyes")
AM_CONDITIONAL(HAVE_ISO_CODES, test "x$isocodes" = "xyes")
AM_CONDITIONAL(WITH_TM, test "x$theme_manager" != "xno")

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
dnl ** GCC FLAGS ********************************************************
dnl *********************************************************************

dnl Only use -Wall and -pipe if we have gcc
if test "x$GCC" = "xyes"; then
	if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
		CFLAGS="$CFLAGS -Wall"
	fi
	dnl these flags might be unwanted
	if test x$minimalflags != xyes; then
		if test "$system" = "Linux" -o "$system" = "FreeBSD"; then
			if test -z "`echo "$CFLAGS" | grep "\-pipe" 2> /dev/null`" ; then
				CFLAGS="$CFLAGS -pipe"
			fi
		fi
		if test -z "`echo "$CFLAGS" | grep "\-g " 2> /dev/null`" ; then
			CFLAGS="$CFLAGS -g"
		fi
	fi
fi

dnl does this compiler support -Wno-pointer-sign ?
AC_MSG_CHECKING([if $CC accepts -Wno-pointer-sign ])

safe_CFLAGS=$CFLAGS
CFLAGS="-Wno-pointer-sign"

AC_TRY_COMPILE(, [
return 0;
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
AC_MSG_CHECKING([if $CC accepts -funsigned-char ])

safe_CFLAGS=$CFLAGS
CFLAGS="-funsigned-char"

AC_TRY_COMPILE(, [
return 0;
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
AC_MSG_CHECKING([if $CC accepts -Wno-unused-result ])

safe_CFLAGS=$CFLAGS
CFLAGS="-Wno-unused-result"

AC_TRY_COMPILE(, [
return 0;
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
data/Makefile
data/icons/Makefile
data/misc/Makefile
data/man/Makefile
data/man/hexchat.1
data/pkgconfig/Makefile
data/pkgconfig/hexchat-plugin.pc
src/Makefile
src/common/Makefile
src/common/dbus/Makefile
src/fe-text/Makefile
src/fe-gtk/Makefile
src/htm/Makefile
src/htm/thememan
osx/Info.plist
plugins/Makefile
plugins/python/Makefile
plugins/perl/Makefile
plugins/checksum/Makefile
plugins/doat/Makefile
plugins/fishlim/Makefile
plugins/sysinfo/Makefile
po/Makefile.in
])

AC_OUTPUT

echo
echo HexChat $VERSION
echo
echo GTK+ interface ........ : $gtkfe
echo Text interface ........ : $textfe
echo Theme manager ......... : $theme_manager
echo
echo OpenSSL support ....... : $openssl
echo D-Bus support ......... : $dbus
echo libnotify support ..... : $libnotify
echo libcanberra support ... : $libcanberra
echo Plugin interface ...... : $plugin
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

echo configure complete, now type \'make\' and pray.
echo
