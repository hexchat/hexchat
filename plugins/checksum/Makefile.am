libdir = $(hexchatlibdir)

lib_LTLIBRARIES = checksum.la
checksum_la_SOURCES = checksum.c
checksum_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
checksum_la_LIBADD = $(GLIB_LIBS)
checksum_la_CPPFLAGS = -I$(top_srcdir)/src/common
checksum_la_CFLAGS = $(GLIB_CFLAGS)
