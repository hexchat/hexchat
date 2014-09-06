libdir = $(hexchatlibdir)

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
python_la_LIBADD = $(PY_LIBS) $(GLIB_LIBS)
python_la_CFLAGS = $(PY_CFLAGS) $(GLIB_CFLAGS) -I$(top_srcdir)/src/common

