## Process this file with automake to produce Makefile.in

LIST =		png_ulist_voice $(srcdir)/ulist_voice.png \
			png_ulist_halfop $(srcdir)/ulist_halfop.png \
			png_ulist_op $(srcdir)/ulist_op.png \
			png_ulist_owner $(srcdir)/ulist_owner.png \
			png_ulist_founder $(srcdir)/ulist_founder.png \
			png_ulist_netop $(srcdir)/ulist_netop.png \
			traymsgpng $(srcdir)/message.png \
			trayhilightpng $(srcdir)/highlight.png \
			trayfilepng $(srcdir)/fileoffer.png \
			bookpng $(srcdir)/book.png \
			channelpng $(srcdir)/channel.png \
			dialogpng $(srcdir)/dialog.png \
			serverpng $(srcdir)/server.png \
			utilpng $(srcdir)/util.png \
			hexchatpng $(srcdir)/../../hexchat.png

PNGS = ulist_voice.png ulist_halfop.png ulist_op.png ulist_owner.png ulist_founder.png ulist_netop.png message.png highlight.png fileoffer.png book.png channel.png dialog.png server.png util.png
noinst_HEADERS = inline_pngs.h
CLEANFILES = $(noinst_HEADERS)
EXTRA_DIST = $(PNGS)

inline_pngs.h: $(PNGS)
	@gdkpixbufcsourcepath@ --raw --build-list $(LIST) > $(srcdir)/inline_pngs.h
