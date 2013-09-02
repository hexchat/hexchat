localedir = $(datadir)/locale

bin_PROGRAMS = hexchat

AM_CPPFLAGS = $(GUI_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

hexchat_LDADD = ../common/libhexchatcommon.a $(GUI_LIBS)

EXTRA_DIST = \
	ascii.h banlist.h chanlist.h chanview.h chanview-tabs.c \
	chanview-tree.c custom-list.h editlist.h fe-gtk.h fkeys.h gtkutil.h joind.h \
	maingui.h menu.h notifygui.h palette.h pixmaps.h \
	plugin-tray.h plugingui.c plugingui.h rawlog.h sexy-iso-codes.h \
	sexy-spell-entry.h textgui.h urlgrab.h userlistgui.h xtext.h \
	../../data/hexchat.gresource.xml

BUILT_SOURCES = resources.c

CLEANFILES = $(BUILT_SOURCES)

if DO_PLUGIN
plugingui_c = plugingui.c
endif

if HAVE_ISO_CODES
iso_codes_c = sexy-iso-codes.c
endif

hexchat_SOURCES = ascii.c banlist.c chanlist.c chanview.c custom-list.c \
	dccgui.c editlist.c fe-gtk.c fkeys.c gtkutil.c ignoregui.c joind.c menu.c \
	maingui.c notifygui.c palette.c pixmaps.c plugin-tray.c $(plugingui_c) \
	rawlog.c resources.c servlistgui.c setup.c $(iso_codes_c) \
	sslalert.c sexy-spell-entry.c textgui.c urlgrab.c userlistgui.c xtext.c
hexchat_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/src/common

resources.c: $(top_srcdir)/data/hexchat.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies $(top_srcdir)/data/hexchat.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/data --generate-source $<
