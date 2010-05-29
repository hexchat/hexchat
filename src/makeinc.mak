#X64 = YES
#PORTABLE = YES
#OPENSSL = YES
IPV6 = YES

CC = cl
LINK = link
CFLAGS = $(CFLAGS) /Ox /c /MD /nologo /DWIN32 /DG_DISABLE_CAST_CHECKS /Dstrcasecmp=stricmp /Dstrncasecmp=strnicmp /Dstrtoull=_strtoui64
CPPFLAGS = /c /MD /nologo /DWIN32
LDFLAGS = /subsystem:windows /nologo
LIBS = $(LIBS) gdi32.lib shell32.lib user32.lib advapi32.lib imm32.lib ole32.lib winmm.lib
!ifdef X64
#############################################################
#x64 config
GLIB = -Ic:/mozilla-build/build/xchat-dev64/include/glib-2.0 -Ic:/mozilla-build/build/xchat-dev64/lib/glib-2.0/include
GTK = -Ic:/mozilla-build/build/xchat-dev64/include/gtk-2.0 -Ic:/mozilla-build/build/xchat-dev64/lib/gtk-2.0/include -Ic:/mozilla-build/build/xchat-dev64/include/atk-1.0 -Ic:/mozilla-build/build/xchat-dev64/include/cairo -Ic:/mozilla-build/build/xchat-dev64/include/pango-1.0 -Ic:/mozilla-build/build/xchat-dev64/include/glib-2.0 -Ic:/mozilla-build/build/xchat-dev64/lib/glib-2.0/include -Ic:/mozilla-build/build/xchat-dev64/include/freetype2 -Ic:/mozilla-build/build/xchat-dev64/include -Ic:/mozilla-build/build/xchat-dev64/include/libpng14
LIBS = $(LIBS) /libpath:c:/mozilla-build/build/xchat-dev64/lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib gdi32.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib

CFLAGS = $(CFLAGS) /favor:AMD64 /D_WIN64
CPPFLAGS = $(CPPFLAGS) /favor:AMD64 /D_WIN64
LDFLAGS = $(LDFLAGS) msvcrt_win2003.obj

PERLPATH = c:\mozilla-build\perl-5.10-x64\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.5-x64
TCLPATH = c:\mozilla-build\tcl-8.6-x64
!else
#############################################################
#x86 config
GLIB = -Ic:/mozilla-build/build/xchat-dev32/include/glib-2.0 -Ic:/mozilla-build/build/xchat-dev32/lib/glib-2.0/include
GTK = -Ic:/mozilla-build/build/xchat-dev32/include/gtk-2.0 -Ic:/mozilla-build/build/xchat-dev32/lib/gtk-2.0/include -Ic:/mozilla-build/build/xchat-dev32/include/atk-1.0 -Ic:/mozilla-build/build/xchat-dev32/include/cairo -Ic:/mozilla-build/build/xchat-dev32/include/pango-1.0 -Ic:/mozilla-build/build/xchat-dev32/include/glib-2.0 -Ic:/mozilla-build/build/xchat-dev32/lib/glib-2.0/include -Ic:/mozilla-build/build/xchat-dev32/include/freetype2 -Ic:/mozilla-build/build/xchat-dev32/include -Ic:/mozilla-build/build/xchat-dev32/include/libpng14
LIBS = $(LIBS) /libpath:c:/mozilla-build/build/xchat-dev32/lib gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gio-2.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib gdi32.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib intl.lib

LDFLAGS = $(LDFLAGS) msvcrt_winxp.obj

PERLPATH = c:\mozilla-build\perl-5.10-x86\lib\CORE
PYTHONPATH = c:\mozilla-build\python-2.5-x86
TCLPATH = c:\mozilla-build\tcl-8.6-x86

MMX = YES
!endif
#############################################################

PERLLIB = perl510
PERLOUTPUT = xcperl.dll

PYTHONLIB = python25
PYTHONOUTPUT = xcpython.dll

TCLLIB = tcl86
TCLOUTPUT = xctcl.dll

!ifdef PORTABLE
CFLAGS = $(CFLAGS) -DPORTABLE_BUILD
!endif

!ifdef IPV6
CFLAGS = $(CFLAGS) -DUSE_IPV6
LIBS = $(LIBS) ws2_32.lib
!else
LIBS = $(LIBS) wsock32.lib
!endif

!ifdef OPENSSL
CFLAGS = $(CFLAGS) /DUSE_OPENSSL
LIBS = $(LIBS) libeay32.lib ssleay32.lib
SSLOBJ = ssl.obj
!endif

COMMON_OBJECTS = \
cfgfiles.obj \
chanopt.obj \
ctcp.obj \
dcc.obj \
history.obj \
identd.obj \
ignore.obj \
inbound.obj \
modes.obj \
network.obj \
notify.obj \
outbound.obj \
plugin.obj \
plugin-timer.obj \
proto-irc.obj \
server.obj \
servlist.obj \
$(SSLOBJ) \
text.obj \
tree.obj \
url.obj \
userlist.obj \
util.obj \
xchat.obj

FEGTK_OBJECTS = \
about.obj \
ascii.obj \
banlist.obj \
chanlist.obj \
chanview.obj \
custom-list.obj \
dccgui.obj \
editlist.obj \
fe-gtk.obj \
fkeys.obj \
gtkutil.obj \
ignoregui.obj \
joind.obj \
maingui.obj \
menu.obj \
notifygui.obj \
palette.obj \
pixmaps.obj \
plugingui.obj \
plugin-tray.obj \
rawlog.obj \
search.obj \
servlistgui.obj \
setup.obj \
sexy-spell-entry.obj \
textgui.obj \
urlgrab.obj \
userlistgui.obj \
xtext.obj

!ifdef MMX
FEGTK_OBJECTS = $(FEGTK_OBJECTS) mmx_cmod.o
CFLAGS = $(CFLAGS) -DUSE_MMX
!endif
