## Process this file with automake to produce Makefile.in

LIST =	bookpng $(srcdir)/book.png \
			hoppng $(srcdir)/hop.png \
			oppng $(srcdir)/op.png \
			purplepng $(srcdir)/purple.png \
			redpng $(srcdir)/red.png \
			voicepng $(srcdir)/voice.png \
			xchatpng $(srcdir)/xchat.png

PNGS = book.png hop.png op.png purple.png red.png voice.png xchat.png
noinst_HEADERS = inline_pngs.h
CLEANFILES = $(noinst_HEADERS)
EXTRA_DIST = $(PNGS) makefile.msc
CONV = @gdkpixbufcsourcepath@

inline_pngs.h:
	$(CONV) --build-list $(LIST) > $(srcdir)/inline_pngs.h
