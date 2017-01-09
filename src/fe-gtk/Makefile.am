
include $(top_srcdir)/m4/clang-analyze.am

localedir = $(datadir)/locale

bin_PROGRAMS = hexchat

AM_CPPFLAGS = $(GUI_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

hexchat_LDADD = $(top_builddir)/src/common/libhexchatcommon.a $(GUI_LIBS)

EXTRA_DIST = \
	ascii.h banlist.h chanlist.h chanview.h chanview-tabs.c \
	chanview-tree.c custom-list.h editlist.h fe-gtk.h fkeys.h gtkutil.h joind.h \
	maingui.h menu.h notifygui.h  notifications palette.h pixmaps.h plugin-notification.h \
	plugin-tray.h plugingui.c plugingui.h rawlog.h servlistgui.h setup.h sexy-iso-codes.h \
	sexy-spell-entry.h textgui.h urlgrab.h userlistgui.h xtext.h

BUILT_SOURCES = resources.h resources.c

CLEANFILES = $(BUILT_SOURCES)

if DO_PLUGIN
plugingui_c = plugingui.c
endif

if HAVE_ISO_CODES
iso_codes_c = sexy-iso-codes.c
endif

if USE_LIBNOTIFY
notify_c = notifications/notification-libnotify.c
else
if HAVE_GTK_MAC
notify_c = notifications/notification-osx.m
hexchat_LDFLAGS = -framework Foundation
else
notify_c = notifications/notification-dummy.c
endif
endif

nodist_hexchat_SOURCES = resources.h resources.c
hexchat_SOURCES = ascii.c banlist.c chanlist.c chanview.c custom-list.c \
	dccgui.c editlist.c fe-gtk.c fkeys.c gtkutil.c ignoregui.c joind.c menu.c \
	maingui.c notifygui.c $(notify_c) palette.c pixmaps.c plugin-tray.c $(plugingui_c) \
	plugin-notification.c rawlog.c servlistgui.c setup.c $(iso_codes_c) \
	sexy-spell-entry.c textgui.c urlgrab.c userlistgui.c xtext.c
hexchat_CPPFLAGS = -I$(top_builddir)/src/common $(AM_CPPFLAGS)

resources_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies $(top_srcdir)/data/hexchat.gresource.xml)

resources.h: $(top_srcdir)/data/hexchat.gresource.xml $(resources_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/data --generate-header --manual-register $<

resources.c: $(top_srcdir)/data/hexchat.gresource.xml $(resources_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/data --generate-source --manual-register $<

EXTRA_DIST += $(resources_files)

if DO_STATIC_ANALYSIS
analyze_plists = $(hexchat_SOURCES:%.c=%.plist)
all-local: $(analyze_plists)
MOSTLYCLEANFILES = $(analyze_plists)
endif
