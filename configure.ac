dnl Process this file with autoconf to produce a configure script.

AC_INIT([HexChat],[2.12.4])

AC_PREREQ([2.64])
AC_COPYRIGHT([Copyright (C) 1998-2010 Peter Zelezny])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11.1 dist-xz no-dist-gzip subdir-objects no-define foreign])
AM_SILENT_RULES([yes])

AC_DEFUN([HEX_CHECK_MACRO], [m4_ifndef([$1], [m4_fatal([macro ]$1[ is not defined. Is ]$2[ installed?])])])

HEX_CHECK_MACRO([AX_APPEND_COMPILE_FLAGS], [autoconf-archive])
HEX_CHECK_MACRO([PKG_PROG_PKG_CONFIG], [pkg-config])

AX_IS_RELEASE([minor-version])
AX_CHECK_ENABLE_DEBUG([yes])

AC_USE_SYSTEM_EXTENSIONS
AM_MAINTAINER_MODE([enable])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_OBJC
AM_PROG_AS
AM_PROG_AR
LT_PREREQ([2.2.6])
LT_INIT([disable-static])
AC_PATH_PROG(MDTOOL, mdtool, no)
PKG_PROG_PKG_CONFIG

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
AH_VERBATIM([USE_LIBPROXY],[#undef USE_LIBPROXY])
AH_VERBATIM([HAVE_LIBPCI],[#undef HAVE_LIBPCI])
AH_VERBATIM([HAVE_ISO_CODES],[#undef HAVE_ISO_CODES])
AH_VERBATIM([HAVE_GTK_MAC],[#undef HAVE_GTK_MAC])
AH_VERBATIM([USE_LIBNOTIFY],[#undef USE_LIBNOTIFY])
AH_VERBATIM([USE_LIBCANBERRA],[#undef USE_LIBCANBERRA])
AH_VERBATIM([USE_OPENSSL],[#undef USE_OPENSSL])
AH_VERBATIM([USE_PLUGIN],[#undef USE_PLUGIN])
AH_VERBATIM([USE_SIGACTION],[#undef USE_SIGACTION])
AH_VERBATIM([USING_FREEBSD],[#undef USING_FREEBSD])
AH_VERBATIM([USING_LINUX],[#undef USING_LINUX])
AH_VERBATIM([socklen_t],[#undef socklen_t])
AH_VERBATIM([USE_DBUS],[#undef USE_DBUS])

AC_PATH_PROG(sedpath, sed)
AS_IF([test "_$sedpath" = _], [
	AC_MSG_ERROR(Cannot find sed: I need it!)
])

AC_PATH_PROG(unamepath, uname)
AS_IF([test "_$unamepath" = _], [
	system="unknown"
], [
	AC_MSG_CHECKING(system type)
	system=`$unamepath -s`
	AC_MSG_RESULT($system)
	AS_IF([test "$system" = "Linux"], [
		AC_DEFINE(USING_LINUX)
	], [test "$system" = "FreeBSD"], [
		AC_DEFINE(USING_FREEBSD)
	])
])

platform_win32=no
platform_osx=no
AS_CASE([$host_os],
	[*mingw*|*cygwin*|*msys*], [
		platform_win32=yes
	],
	[darwin*], [
		platform_osx=yes
	]
)

dnl *********************************************************************
dnl ** configure switches ***********************************************
dnl *********************************************************************

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

AC_ARG_ENABLE(lua,
	[AS_HELP_STRING([--disable-lua],[don\'t build the lua plugin])],
        lua=$enableval, lua=yes)

AC_ARG_ENABLE(perl_old,
	[AS_HELP_STRING([--disable-perl_old],[no backwards compatibility for perl plugin])],
        perl_old=$enableval, perl_old=yes)

AC_ARG_ENABLE(plugin,
	[AS_HELP_STRING([--disable-plugin],[disable plugin support])],
        plugin=$enableval, plugin=yes)

AC_ARG_ENABLE(checksum,
	[AS_HELP_STRING([--disable-checksum],[disable the Checksum plugin])],
        checksum=$enableval, checksum=yes)

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

AC_ARG_ENABLE(libproxy,
	[AS_HELP_STRING([--disable-libproxy],[disable libproxy support (default: auto)])],
        libproxy=$enableval, libproxy=auto)

AC_ARG_ENABLE(isocodes,
	[AS_HELP_STRING([--disable-isocodes],[disable iso-codes with spell-check])],
        isocodes=$enableval, isocodes=yes)

AC_ARG_ENABLE(static-analysis,
	[AS_HELP_STRING([--enable-static-analysis],[if using clang run static analysis during build (default: no)])],
        analyze=$enableval, analyze=no)

AC_ARG_WITH(theme-manager,
	[AS_HELP_STRING([--with-theme-manager],[compile theme manager (needs monodevelop, default: off)])],
		theme_manager=$withval, theme_manager=no)

AC_ARG_ENABLE(stack-protector,
	[AS_HELP_STRING([--disable-stack-protector],[disable building with stack-protector])],
        stack_protector=$enableval, stack_protector=yes)


dnl *********************************************************************
dnl ** THEME-MANAGER ****************************************************
dnl *********************************************************************
AS_IF([test "x$theme_manager" != "xno"], [
	AS_IF([test "x$MDTOOL" = "xno"], [
		AC_MSG_ERROR([No "mdtool" found, you need to install monodevelop!])
	])
])


dnl *********************************************************************
dnl ** GLIB *************************************************************
dnl *********************************************************************

AM_PATH_GLIB_2_0([2.34.0], [], [AC_MSG_ERROR([Glib not found!])], [gmodule gobject gio])
COMMON_CFLAGS="$GLIB_CFLAGS -DG_DISABLE_SINGLE_INCLUDES"
COMMON_LIBS="$GLIB_LIBS"
AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [GLIB_VERSION_2_34], [Dont warn using older APIs])
AC_DEFINE([GLIB_VERSION_MAX_ALLOWED], [GLIB_VERSION_2_34], [Prevents using newer APIs])

dnl *********************************************************************
dnl ** GTK **************************************************************
dnl *********************************************************************

AS_IF([test "$gtkfe" = yes], [
	PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.24.0], [
		GUI_LIBS="$GUI_LIBS $GTK_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $GTK_CFLAGS -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED"
	], [
		gtkfe=no
	])
])

dnl *********************************************************************
dnl ** MAC_INTEGRATION **************************************************
dnl *********************************************************************

_gdk_tgt=`$PKG_CONFIG --variable=target gdk-2.0`
AS_IF([test "x$_gdk_tgt" = xquartz], [
	PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration-gtk2, [
		GUI_LIBS="$GUI_LIBS $GTK_MAC_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $GTK_MAC_CFLAGS"
		AC_DEFINE(HAVE_GTK_MAC)
	])
])


dnl *********************************************************************
dnl ** Lua **************************************************************
dnl *********************************************************************

AS_IF([test "$lua" = yes], [
	AC_MSG_CHECKING(for plugin interface used by lua)
	AS_IF([test "$plugin" = yes], [
		AC_MSG_RESULT([yes])

		m4_define_default([_LUA_LIST], [luajit lua lua5.3 lua53 lua5.2 lua52 lua5.1 lua51])

		AC_ARG_VAR([LUA], [The Lua pkgconfig name, e.g. luajit or lua5.2])
		AS_IF([test "x$LUA" = 'x'], [
			for lua_var in _LUA_LIST; do
				$PKG_CONFIG --exists $lua_var || continue
				LUA=$lua_var
				break
			done
			AS_IF([test "x$LUA" = 'x'], [
				AC_MSG_ERROR([Failed to find lua])
			])
		])

		PKG_CHECK_MODULES([LUA], $LUA, [
			AC_SUBST([LUA_CFLAGS])
			AC_SUBST([LUA_LIBS])
		])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for lua])
		lua=no
	])
])

dnl *********************************************************************
dnl ** PERL *************************************************************
dnl *********************************************************************

AS_IF([test "$perl" = yes], [
	AC_MSG_CHECKING(for plugin interface used by Perl)
	AS_IF([test "$plugin" = yes], [
		AC_MSG_RESULT([yes])

		AX_PERL_EXT_FLAGS([PERL_CFLAGS], [PERL_LDFLAGS])
		original_cflags="$CFLAGS"
		original_ldflags="$LDFLAGS"
		CFLAGS="$PERL_CFLAGS"
		LDFLAGS="$PERL_LDFLAGS"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
				#define PERL_NO_INLINE_FUNCTIONS
				#include <EXTERN.h>
				#include <perl.h>
			]], [[]])],[perl_is_usable=yes],[perl_is_usable=no])
		CFLAGS="$original_cflags"
		LDFLAGS="$original_ldflags"
		AS_IF([test "$perl_is_usable" = "no"], [
			AC_MSG_WARN([perl test failed to compile, disabling])
			perl=no
		], [
			AC_MSG_CHECKING([if perl plugin will be backward compatible])
			AS_IF([test "$perl_old" = "yes"], [
				  AC_MSG_RESULT([yes])
				  AC_DEFINE(OLD_PERL)
			], [
				  AC_MSG_RESULT([no])
			])
		])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for Perl])
		perl=no
	])
])

dnl *********************************************************************
dnl ** PYTHON ***********************************************************
dnl *********************************************************************

AS_IF([test "x$python" != xno], [
	AC_MSG_CHECKING(for plugin interface used by Python)
	AS_IF([test "$plugin" = yes], [
		AC_MSG_RESULT([yes])
		AS_CASE([$python],
			dnl set python2 default here
			[python2], [
				PYTHON_VERSION=2
				AX_PYTHON_DEVEL([>= '2.7'])
			],
			dnl set python3 default here
			[python3], [
				PYTHON_VERSION=3
				AX_PYTHON_DEVEL([>= '3.3'])
			],
			dnl add broken versions here
			[python2.5|python2.6|python3.1|python3.2], [
				AC_MSG_WARN(Unsupported Python version ${python}!)
			],
			dnl user supplied version
			[python*], [
				PYTHON_VERSION="${python#python}"
				AX_PYTHON_DEVEL()
			],[
				AC_MSG_WARN(Unsupported Python ${python}!)
			]
		)
		AC_MSG_CHECKING(Python version)

		AS_IF([test "$ac_python_version" != ""], [
			AC_MSG_RESULT($ac_python_version)
			python="python-${ac_python_version}"
		], [
			AC_MSG_RESULT(Not found)
			python=no
		])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for Python])
		python=no
	])
])

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

AC_CHECK_FUNCS(getaddrinfo, have_getaddrinfo=yes)
AC_MSG_CHECKING(whether IPv6 is supported)
AS_IF([test "$have_getaddrinfo" = yes], [
	AC_MSG_RESULT(yes)
], [
	AC_MSG_RESULT(no)
	AC_MSG_ERROR(ipv6 support not found!)
])

dnl *********************************************************************
dnl ** OPENSSL **********************************************************
dnl *********************************************************************


AS_IF([test "$openssl" != no], [
	PKG_CHECK_MODULES(OPENSSL, [openssl >= 0.9.8], [
		AC_DEFINE(USE_OPENSSL)
		openssl=yes
		COMMON_LIBS="$COMMON_LIBS $OPENSSL_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $OPENSSL_CFLAGS"
	], [
		unset openssl_path ac_cv_lib_ssl_SSL_new ac_cv_header_openssl_ssl_h
		AS_IF([test "$openssl" != yes], [
			openssl_path=$openssl
		])
		openssl=no
		OPENSSL_LIBS="-lcrypto"
		AS_IF([test -n "$openssl_path"], [
			OPENSSL_LIBS="-L$openssl_path/lib $OPENSSL_LIBS"
		])
		SAVED_LIBS=$LIBS
		LIBS="$LIBS $OPENSSL_LIBS"
		AC_CHECK_LIB(ssl, SSL_new, [
			AS_IF([test -n "$openssl_path"], [
				OPENSSL_CFLAGS="-I$openssl_path/include"
			])
			SAVED_CFLAGS=$CFLAGS
			CFLAGS="$CFLAGS $OPENSSL_CFLAGS"
			AC_CHECK_HEADERS(openssl/ssl.h, [
				openssl=yes
				AC_DEFINE(USE_OPENSSL)
				OPENSSL_LIBS="$OPENSSL_LIBS -lssl"

				COMMON_LIBS="$COMMON_LIBS $OPENSSL_LIBS"
				COMMON_CFLAGS="$COMMON_CFLAGS $OPENSSL_CFLAGS"
			])
			CFLAGS=$SAVED_CFLAGS
		])
		LIBS=$SAVED_LIBS
	])

	AS_IF([test "$openssl" = yes], [
		dnl Test for various functions that are not available in LibreSSL
		SAVED_LIBS="$LIBS"
		LIBS="$OPENSSL_LIBS"
		AC_CHECK_FUNCS([SSL_CTX_get_ssl_method X509_get_signature_nid DH_set0_pqg DH_get0_key DH_set0_key])
		LIBS="$SAVED_LIBS"
	])
])

dnl *********************************************************************
dnl ** LIBPROXY *********************************************************
dnl *********************************************************************

AS_IF([test "x$libproxy" = "xyes" -o "x$libproxy" = "xauto"], [
	PKG_CHECK_MODULES([LIBPROXY], [libproxy-1.0], [
		COMMON_LIBS="$COMMON_LIBS $LIBPROXY_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $LIBPROXY_CFLAGS"
		AC_DEFINE(USE_LIBPROXY)
		libproxy=yes
	], [
		AS_IF([test "x$libproxy" = "xyes"], [
			AC_MSG_ERROR(Cannot find libproxy!)
		])
		libproxy=no
	])
], [
	libproxy=no
])

dnl *********************************************************************
dnl ** PLUGIN ***********************************************************
dnl *********************************************************************

AS_IF([test "$plugin" = yes], [
	AC_DEFINE(USE_PLUGIN)
	PLUGIN_LDFLAGS="-avoid-version"
	AS_IF([test "$platform_win32" = yes], [
		PLUGIN_LDFLAGS="$PLUGIN_LDFLAGS -no-undefined"
	])
])

dnl *********************************************************************
dnl ** Checksum *********************************************************
dnl *********************************************************************

AS_IF([test "$checksum" != "no"], [
	checksum=no
	AC_MSG_CHECKING(for plugin interface used by Checksum)
	AS_IF([test "$plugin" = yes], [
		checksum=yes
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option])
	])
])

dnl *********************************************************************
dnl ** FiSHLiM **********************************************************
dnl *********************************************************************

AS_IF([test "$fishlim" != "no"], [
	fishlim=no
	AC_MSG_CHECKING(for plugin interface used by FiSHLiM)
	AS_IF([test "$plugin" = yes], [
		AC_MSG_RESULT([yes])
		AC_MSG_CHECKING(for OpenSSL used by FiSHLiM)
		AS_IF([test "$openssl" = yes], [
			fishlim=yes
			AC_MSG_RESULT([yes])
		], [
			AC_MSG_RESULT([OpenSSL cannot be found, use the --enable-openssl option])
		])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option])
	])
])

dnl *********************************************************************
dnl ** SYSINFO **********************************************************
dnl *********************************************************************

AS_IF([test "$sysinfo" != "no"], [
	AC_MSG_CHECKING(for plugin interface used by SysInfo)
	AS_IF([test "$plugin" = yes], [
		AC_MSG_RESULT([yes])
		AS_IF([test "$platform_osx" = yes], [
			sysinfo=yes
		], [
			PKG_CHECK_MODULES(LIBPCI, libpci >= 3.0.0, [
				sysinfo=yes
				AC_DEFINE(HAVE_LIBPCI)
			], [sysinfo=no])
		])
	], [
		AC_MSG_RESULT([plugins are disabled, use the --enable-plugin option for SysInfo])
		sysinfo=no
	])
])

dnl #######################################################################
dnl # Check for DBUS libraries
dnl #######################################################################

AS_IF([test "x$dbus" = "xyes"], [
	PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.60 dbus-glib-1 >= 0.60 gthread-2.0], dbus=yes, [
		dbus=no
	])
	AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool, no)
	AS_IF([test "x$DBUS_BINDING_TOOL" = "xno" || test "x$dbus" = "xno"], [
		dbus="no"
	], [
		COMMON_LIBS="$COMMON_LIBS $DBUS_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $DBUS_CFLAGS"
		AC_DEFINE(USE_DBUS)

		AS_AC_EXPAND(DBUS_SERVICES_DIR, "$datadir/dbus-1/services")
		AC_SUBST(DBUS_SERVICES_DIR)
		AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBUS is])
	])
])

dnl *********************************************************************
dnl ** LIBNOTIFY ********************************************************
dnl *********************************************************************

AS_IF([test "x$libnotify" = "xyes"], [
	PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.4, [], [
		libnotify=no
	])
	AS_IF([test "$libnotify" != "no"], [
		GUI_LIBS="$GUI_LIBS $LIBNOTIFY_LIBS"
		GUI_CFLAGS="$GUI_CFLAGS $LIBNOTIFY_CFLAGS"
		AC_DEFINE(USE_LIBNOTIFY)
	])
])

dnl *********************************************************************
dnl ** LIBCANBERRA ******************************************************
dnl *********************************************************************

AS_IF([test "x$libcanberra" = "xyes"], [
	PKG_CHECK_MODULES(LIBCANBERRA, libcanberra >= 0.22, [], [
		libcanberra=no
	])
	AS_IF([test "$libcanberra" != "no"], [
		COMMON_LIBS="$COMMON_LIBS $LIBCANBERRA_LIBS"
		COMMON_CFLAGS="$COMMON_CFLAGS $LIBCANBERRA_CFLAGS"
		AC_DEFINE(USE_LIBCANBERRA)
	])
])

dnl *********************************************************************
dnl ** SPELL ************************************************************
dnl *********************************************************************

AS_IF([test "x$isocodes" = "xyes"], [
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
])

dnl *********************************************************************
dnl ** Static Analysis **************************************************
dnl *********************************************************************

AS_IF([test "x$analyze" = "xyes"], [
	AS_IF([test "$CC" != "clang"], [
		AC_MSG_WARN(CC is not clang for static analysis)
		analyze=no
	])
])

dnl *********************************************************************
dnl ** CONDITIONALS *****************************************************
dnl *********************************************************************

AM_CONDITIONAL(USE_OPENSSL, test "x$openssl" = "xyes")
AM_CONDITIONAL(USE_LIBNOTIFY, test "x$libnotify" = "xyes")
AM_CONDITIONAL(USE_LIBCANBERRA, test "x$libcanberra" = "xyes")
AM_CONDITIONAL(DO_TEXT, test "x$textfe" = "xyes")
AM_CONDITIONAL(DO_GTK, test "x$gtkfe" = "xyes")
AM_CONDITIONAL(DO_PERL, test "x$perl" = "xyes")
AM_CONDITIONAL(DO_LUA, test "x$lua" = "xyes")
AM_CONDITIONAL(DO_PYTHON, test "x$python" != "xno")
AM_CONDITIONAL(DO_PLUGIN, test "x$plugin" = "xyes")
AM_CONDITIONAL(DO_CHECKSUM, test "x$checksum" = "xyes")
AM_CONDITIONAL(DO_FISHLIM, test "x$fishlim" = "xyes")
AM_CONDITIONAL(DO_SYSINFO, test "x$sysinfo" = "xyes")
AM_CONDITIONAL(DO_STATIC_ANALYSIS, test "x$analyze" = "xyes")
AM_CONDITIONAL(USE_DBUS, test "x$dbus" = "xyes")
AM_CONDITIONAL(HAVE_ISO_CODES, test "x$isocodes" = "xyes")
AM_CONDITIONAL(HAVE_GTK_MAC, test "x$_gdk_tgt" = xquartz)
AM_CONDITIONAL(WITH_TM, test "x$theme_manager" != "xno")
AM_CONDITIONAL(PLATFORM_OSX, test "x$platform_osx" = "xyes")

dnl *********************************************************************
dnl ** CFLAGS ***********************************************************
dnl *********************************************************************

AX_APPEND_COMPILE_FLAGS([\
	-pipe \
	-funsigned-char \
	-fPIE \
	-fPIC \
	-Wall \
	-Wextra \
	-Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-Wno-pointer-sign \
	-Wno-missing-field-initializers \
	-Wno-unused-result \
	-Werror=format-security \
	-Werror=init-self \
	-Werror=declaration-after-statement \
	-Werror=missing-include-dirs \
	-Werror=date-time \
	-Werror=implicit-function-declaration \
	-Werror=pointer-arith \
])

AS_IF([test "$stack_protector" = "yes"], [
	AX_APPEND_COMPILE_FLAGS([ \
		-fstack-protector-strong \
	])
])

AX_APPEND_LINK_FLAGS([ \
	-pie \
	-Wl,-z,relro \
	-Wl,-z,now \
])

dnl *********************************************************************
dnl ** FUNCTIONS/LIBS/CFLAGS ********************************************
dnl *********************************************************************

AC_MSG_CHECKING(for modern sigaction)
dnl libc5 on linux and FreeBSD 3.x doesn\'t have siginfo_t
dnl and the sa_sigation field.
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[struct sigaction act;
	 siginfo_t *si;
	 act.sa_sigaction = 0;]])],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(USE_SIGACTION) 
	],[AC_MSG_RESULT(no)])

AC_CHECK_FUNCS(memrchr)

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
AS_IF([test $ac_cv_type_socklen_t = no], [
	AC_DEFINE(socklen_t, int)
])

dnl Mac OS X and Darwin use lookupd, which caches DNS queries by default
AC_EGREP_CPP(lookupd, dnl
[#if (defined(__APPLE__) && defined(__MACH__))
	lookupd
#endif], AC_DEFINE([LOOKUPD],1,[Define to 1 if the system uses lookupd]))

dnl freebsd needs this
LIBS="$LIBS $INTLLIBS"

GUI_LIBS="$GUI_LIBS $COMMON_LIBS"

dnl make these visible to all Makefiles
AC_SUBST(GUI_LIBS)
AC_SUBST(GUI_CFLAGS)
AC_SUBST(COMMON_LIBS)
AC_SUBST(COMMON_CFLAGS)
AC_SUBST(PERL_CFLAGS)
AC_SUBST(PERL_LDFLAGS)
AC_SUBST(PYTHON_CPPFLAGS)
AC_SUBST(PYTHON_LIBS)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
AC_SUBST(OPENSSL_LIBS)
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(PLUGIN_LDFLAGS)

m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR], AC_SUBST([pkgconfigdir], ${libdir}/pkgconfig))

dnl for plugin.c and pixmaps.c
AS_IF([test "x$prefix" = xNONE], [
	prefix="$ac_default_prefix"
])
AS_IF([test "x$exec_prefix" = xNONE], [
	exec_prefix="$prefix"
])

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
plugins/lua/Makefile
plugins/python/Makefile
plugins/perl/Makefile
plugins/checksum/Makefile
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
echo libproxy support ...... : $libproxy
echo
echo Lua ................... : $lua \($LUA\)
echo Perl .................. : $perl
echo Python ................ : $python
echo
echo Checksum .............. : $checksum
echo FiSHLiM ............... : $fishlim
echo SysInfo ............... : $sysinfo
echo
echo Debug mode ............ : $enable_debug
echo
echo The binary will be installed in $prefix/bin
echo

if test "$gtkfe" = no; then
	echo Warning: The GTK \(GUI\) frontend will not be built.
	echo
fi

echo configure complete, now type \'make\' and pray.
echo
