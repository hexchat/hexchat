localedir = $(datadir)/locale

bin_PROGRAMS = hexchat-text

EXTRA_DIST = README 

AM_CPPFLAGS = $(COMMON_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

hexchat_text_LDADD = ../common/libhexchatcommon.a $(COMMON_LIBS)
hexchat_text_SOURCES = fe-text.c fe-text.h

