## Process this file with automake to produce Makefile.in

PNGS = book.png hop.png op.png purple.png red.png voice.png xchat.png
noinst_HEADERS = book.h hop.h op.h purple.h red.h voice.h xchat.h
CLEANFILES = $(noinst_HEADERS)
EXTRA_DIST = $(PNGS) makefile.msc
CONV = @gdkpixbufcsourcepath@
SUFFIXES = .png .h

.png.h:
	$(CONV) --name=$(patsubst %.h,%png,$@) $< > $@
