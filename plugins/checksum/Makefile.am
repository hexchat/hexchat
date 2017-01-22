AM_CFLAGS = @hexchat_def_CFLAGS@
AM_LDFLAGS = @hexchat_def_LDFLAGS@

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = checksum.la
checksum_la_SOURCES = checksum.c
checksum_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module @hexchat_def_LDFLAGS@
checksum_la_LIBADD = $(GLIB_LIBS)
checksum_la_CPPFLAGS = -I$(top_srcdir)/src/common
checksum_la_CFLAGS = $(GLIB_CFLAGS) @hexchat_def_CFLAGS@
