localedir = $(datadir)/locale

bin_PROGRAMS = xchat

INCLUDES = $(GUI_CFLAGS) -DG_DISABLE_CAST_CHECKS -DLOCALEDIR=\"$(localedir)\"

xchat_LDADD = ../common/libxchatcommon.a $(GUI_LIBS)

EXTRA_DIST = makefile.msc \
	about.h ascii.h banlist.h chanlist.h editlist.h fe-gtk.h fkeys.h \
	gtkutil.h maingui.h menu.h mmx_cmod.S mmx_cmod.h notifygui.h palette.h \
	pixmaps.h plugingui.c plugingui.h rawlog.h search.h \
	tabs.h textgui.h urlgrab.h userlistgui.h xtext.h

if USE_MMX
mmx_cmod_S = mmx_cmod.S
endif

if DO_PLUGIN
plugingui_c = plugingui.c
endif

xchat_SOURCES = about.c ascii.c banlist.c chanlist.c dccgui.c editlist.c \
	fe-gtk.c fkeys.c gtkutil.c ignoregui.c menu.c maingui.c $(mmx_cmod_S) \
	notifygui.c palette.c pixmaps.c $(plugingui_c) rawlog.c \
	search.c servlistgui.c setup.c tabs.c textgui.c \
	urlgrab.c userlistgui.c xtext.c

pixmaps.o: $(srcdir)/pixmaps.c
	$(COMPILE) -DXCHATSHAREDIR=\"$(xchatsharedir)\" -c $(srcdir)/pixmaps.c
