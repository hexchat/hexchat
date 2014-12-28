libdir = $(hexchatlibdir)

lib_LTLIBRARIES = doat.la
doat_la_SOURCES = doat.c
doat_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
doat_la_LIBADD = $(GLIB_LIBS)
doat_la_CFLAGS = $(GLIB_CFLAGS) -I$(top_srcdir)/src/common

