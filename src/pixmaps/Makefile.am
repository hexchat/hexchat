## Process this file with automake to produce Makefile.in

LIST =		png_ulist_voice $(srcdir)/ulist_voice.png \
			png_ulist_halfop $(srcdir)/ulist_halfop.png \
			png_ulist_op $(srcdir)/ulist_op.png \
			png_ulist_owner $(srcdir)/ulist_owner.png \
			png_ulist_founder $(srcdir)/ulist_founder.png \
			png_ulist_netop $(srcdir)/ulist_netop.png \
			png_tray_fileoffer $(srcdir)/tray_fileoffer.png \
			png_tray_highlight $(srcdir)/tray_highlight.png \
			png_tray_message $(srcdir)/tray_message.png \
			png_tree_channel $(srcdir)/tree_channel.png \
			png_tree_dialog $(srcdir)/tree_dialog.png \
			png_tree_server $(srcdir)/tree_server.png \
			png_tree_util $(srcdir)/tree_util.png \
			png_book $(srcdir)/book.png \
			png_hexchat $(srcdir)/hexchat.png

PNGS = ulist_voice.png ulist_halfop.png ulist_op.png ulist_owner.png ulist_founder.png ulist_netop.png tray_fileoffer.png tray_highlight.png tray_message.png tree_channel.png tree_dialog.png tree_server.png tree_util.png book.png
noinst_HEADERS = inline_pngs.h
CLEANFILES = $(noinst_HEADERS)
EXTRA_DIST = $(PNGS)

inline_pngs.h: $(PNGS)
	@gdkpixbufcsourcepath@ --raw --build-list $(LIST) > $(srcdir)/inline_pngs.h
