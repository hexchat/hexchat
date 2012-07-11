CC = cl
LINK = link
CFLAGS = $(CFLAGS) /Ox /c /MD /MP /W0 /nologo
CFLAGS = $(CFLAGS) /DWIN32 /DG_DISABLE_CAST_CHECKS /DG_DISABLE_DEPRECATED /DGDK_PIXBUF_DISABLE_DEPRECATED /DGDK_DISABLE_DEPRECATED /DHAVE_STRTOULL /Dstrtoull=_strtoui64 /Dstrcasecmp=stricmp /Dstrncasecmp=strnicmp
CFLAGS = $(CFLAGS) /I$(DEV)\include
CPPFLAGS = /c /MD /W0 /nologo /DWIN32
LDFLAGS = /subsystem:windows /nologo
LIBS = $(LIBS) gdi32.lib shell32.lib user32.lib advapi32.lib imm32.lib ole32.lib winmm.lib ws2_32.lib wininet.lib comdlg32.lib libeay32.lib ssleay32.lib

GLIB = /I$(DEV)\include\glib-2.0 /I$(DEV)\lib\glib-2.0\include /I$(DEV)\include\libxml2
GTK = /I$(DEV)\include\gtk-2.0 /I$(DEV)\lib\gtk-2.0\include /I$(DEV)\include\atk-1.0 /I$(DEV)\include\cairo /I$(DEV)\include\pango-1.0 /I$(DEV)\include\gdk-pixbuf-2.0
LIBS = $(LIBS) /libpath:$(DEV)\lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib libxml2.lib
#obs LIBS = $(LIBS) /libpath:$(DEV)\lib libgtk-win32-2.0-0.lib libgdk-win32-2.0-0.lib libatk-1.0-0.lib libgio-2.0-0.lib libgdk_pixbuf-2.0-0.lib libpangowin32-1.0-0.lib libpangocairo-1.0-0.lib libpango-1.0-0.lib libcairo-2.lib libgobject-2.0-0.lib libgmodule-2.0-0.lib libglib-2.0-0.lib libintl-8.lib libxml2-2.lib

LUALIB = lua51
LUAOUTPUT = xclua.dll

PERL512LIB = perl512
PERL512OUTPUT = xcperl-512.dll
PERL514LIB = perl514
PERL514OUTPUT = xcperl-514.dll

PYTHONLIB = python27
PYTHONOUTPUT = xcpython.dll

TCLLIB = tcl85
TCLOUTPUT = xctcl.dll

!ifdef X64
CFLAGS = $(CFLAGS) /favor:AMD64 /D_WIN64
CPPFLAGS = $(CPPFLAGS) /favor:AMD64 /D_WIN64
LDFLAGS = $(LDFLAGS) msvcrt_win2003.obj

PERL512PATH = c:\mozilla-build\perl-5.12-x64
PERL514PATH = c:\mozilla-build\perl-5.14-x64
PYTHONPATH = c:\mozilla-build\python-2.7-x64
TCLPATH = c:\mozilla-build\tcl-8.5-x64
!else
LDFLAGS = $(LDFLAGS) msvcrt_winxp.obj

PERL512PATH = c:\mozilla-build\perl-5.12-x86
PERL514PATH = c:\mozilla-build\perl-5.14-x86
PYTHONPATH = c:\mozilla-build\python-2.7-x86
TCLPATH = c:\mozilla-build\tcl-8.5-x86
!endif
