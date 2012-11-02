## Process this file with automake to produce Makefile.in

LIST =	traymsgpng $(srcdir)/message.png \
			trayhilightpng $(srcdir)/highlight.png \
			trayfilepng $(srcdir)/fileoffer.png \
			bookpng $(srcdir)/book.png \
			hoppng $(srcdir)/hop.png \
			oppng $(srcdir)/op.png \
			orangepng $(srcdir)/orange.png \
			purplepng $(srcdir)/purple.png \
			redpng $(srcdir)/red.png \
			voicepng $(srcdir)/voice.png \
			channelpng $(srcdir)/channel.png \
			dialogpng $(srcdir)/dialog.png \
			serverpng $(srcdir)/server.png \
			utilpng $(srcdir)/util.png \
			hexchatpng $(srcdir)/../../hexchat.png

PNGS = message.png highlight.png fileoffer.png book.png hop.png op.png orange.png purple.png red.png voice.png channel.png dialog.png server.png util.png
noinst_HEADERS = inline_pngs.h
CLEANFILES = $(noinst_HEADERS)
EXTRA_DIST = $(PNGS)

inline_pngs.h: $(PNGS)
	@gdkpixbufcsourcepath@ --raw --build-list $(LIST) > $(srcdir)/inline_pngs.h
