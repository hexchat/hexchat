localedir = $(datadir)/locale

bin_PROGRAMS = hexchat-text

AM_CPPFLAGS = $(COMMON_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

hexchat_text_LDADD = $(top_builddir)/src/common/libhexchatcommon.a $(COMMON_LIBS)
hexchat_text_SOURCES = fe-text.c fe-text.h

