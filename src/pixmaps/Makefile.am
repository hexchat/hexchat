## Process this file with automake to produce Makefile.in

PNGS = book.png hop.png op.png purple.png red.png voice.png xchat.png
nodist_pkgdata_DATA = book.h hop.h op.h purple.h red.h voice.h xchat.h
CLEANFILES = $(nodist_pkgdata_DATA)
EXTRA_DIST = $(PNGS)
CONV = @gdkpixbufcsourcepath@
SUFFIXES = .png .h

.png.h:
	$(CONV) --name=$(patsubst %.h,%png,$@) $< > $@
