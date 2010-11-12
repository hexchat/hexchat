CC = cl
LINK = link
CFLAGS = $(CFLAGS) /Ox /c /MD /MP2 /W0 /nologo
CFLAGS = $(CFLAGS) /DWIN32 /DG_DISABLE_CAST_CHECKS /DG_DISABLE_DEPRECATED /DGDK_PIXBUF_DISABLE_DEPRECATED /DGDK_DISABLE_DEPRECATED /DUSE_IPV6 /DHAVE_STRTOULL /Dstrtoull=_strtoui64 /Dstrcasecmp=stricmp /Dstrncasecmp=strnicmp /DUSE_OPENSSL
CFLAGS = $(CFLAGS)
CPPFLAGS = /c /MD /W0 /nologo /DWIN32
LDFLAGS = /subsystem:windows /nologo
LIBS = $(LIBS) gdi32.lib shell32.lib user32.lib advapi32.lib imm32.lib ole32.lib winmm.lib ws2_32.lib wininet.lib comdlg32.lib libeay32.lib ssleay32.lib

!ifdef X64
#############################################################
#x64 config

GLIB = /I$(DEV64)\include\glib-2.0 /I$(DEV64)\lib\glib-2.0\include
GTK = /I$(DEV64)\include\gtk-2.0 /I$(DEV64)\lib\gtk-2.0\include /I$(DEV64)\include\atk-1.0 /I$(DEV64)\include\cairo /I$(DEV64)\include\pango-1.0 /I$(DEV64)\include\gdk-pixbuf-2.0
LIBS = $(LIBS) /libpath:$(DEV64)\lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib

CFLAGS = $(CFLAGS) /favor:AMD64 /D_WIN64 /I$(DEV64)\include
CPPFLAGS = $(CPPFLAGS) /favor:AMD64 /D_WIN64 /I$(DEV64)\include
LDFLAGS = $(LDFLAGS) msvcrt_win2003.obj

PERL510PATH = c:\mozilla-build\perl-5.10-x64\lib\CORE
PERL512PATH = c:\mozilla-build\perl-5.12-x64\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.6-x64
TCLPATH = c:\mozilla-build\tcl-8.5-x64
!else
#############################################################
#x86 config

GLIB = /I$(DEV32)\include\glib-2.0 /I$(DEV32)\lib\glib-2.0\include
GTK = /I$(DEV32)\include\gtk-2.0 /I$(DEV32)\lib\gtk-2.0\include /I$(DEV32)\include\atk-1.0 /I$(DEV32)\include\cairo /I$(DEV32)\include\pango-1.0 /I$(DEV32)\include\gdk-pixbuf-2.0
LIBS = $(LIBS) /libpath:$(DEV32)\lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib

CFLAGS = $(CFLAGS) /I$(DEV32)\include
LDFLAGS = $(LDFLAGS) msvcrt_winxp.obj

PERL510PATH = c:\mozilla-build\perl-5.10-x86\lib\CORE
PERL512PATH = c:\mozilla-build\perl-5.12-x86\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.6-x86
TCLPATH = c:\mozilla-build\tcl-8.5-x86
!endif
#############################################################

LUALIB = lua51
LUAOUTPUT = xclua.dll

PYTHONLIB = python26
PYTHONOUTPUT = xcpython.dll

TCLLIB = tcl85
TCLOUTPUT = xctcl.dll
