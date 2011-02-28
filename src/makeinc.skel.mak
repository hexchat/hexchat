CC = cl
LINK = link
CFLAGS = $(CFLAGS) /Ox /c /MD /MP2 /W0 /nologo
CFLAGS = $(CFLAGS) /DWIN32 /DG_DISABLE_CAST_CHECKS /DG_DISABLE_DEPRECATED /DGDK_PIXBUF_DISABLE_DEPRECATED /DGDK_DISABLE_DEPRECATED /DUSE_IPV6 /DHAVE_STRTOULL /Dstrtoull=_strtoui64 /Dstrcasecmp=stricmp /Dstrncasecmp=strnicmp /DUSE_OPENSSL
CFLAGS = $(CFLAGS) /I$(DEV)\include
CPPFLAGS = /c /MD /W0 /nologo /DWIN32
LDFLAGS = /subsystem:windows /nologo
LIBS = $(LIBS) gdi32.lib shell32.lib user32.lib advapi32.lib imm32.lib ole32.lib winmm.lib ws2_32.lib wininet.lib comdlg32.lib libeay32.lib ssleay32.lib

GLIB = /I$(DEV)\include\glib-2.0 /I$(DEV)\lib\glib-2.0\include
GTK = /I$(DEV)\include\gtk-2.0 /I$(DEV)\lib\gtk-2.0\include /I$(DEV)\include\atk-1.0 /I$(DEV)\include\cairo /I$(DEV)\include\pango-1.0 /I$(DEV)\include\gdk-pixbuf-2.0
LIBS = $(LIBS) /libpath:$(DEV)\lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib

LUALIB = lua51
LUAOUTPUT = xclua.dll

PERL510LIB = perl510
PERL510OUTPUT = xcperl-510.dll
PERL512LIB = perl512
PERL512OUTPUT = xcperl-512.dll

PYTHONLIB = python27
PYTHONOUTPUT = xcpython.dll

TCLLIB = tcl85
TCLOUTPUT = xctcl.dll

!ifdef X64
CFLAGS = $(CFLAGS) /favor:AMD64 /D_WIN64
CPPFLAGS = $(CPPFLAGS) /favor:AMD64 /D_WIN64
LDFLAGS = $(LDFLAGS) msvcrt_win2003.obj

PERL510PATH = c:\mozilla-build\perl-5.10-x64\lib\CORE
PERL512PATH = c:\mozilla-build\perl-5.12-x64\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.7-x64
TCLPATH = c:\mozilla-build\tcl-8.5-x64
!else
LDFLAGS = $(LDFLAGS) msvcrt_winxp.obj

PERL510PATH = c:\mozilla-build\perl-5.10-x86\lib\CORE
PERL512PATH = c:\mozilla-build\perl-5.12-x86\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.7-x86
TCLPATH = c:\mozilla-build\tcl-8.5-x86
!endif
