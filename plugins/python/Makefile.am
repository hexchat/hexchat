libdir = $(hexchatlibdir)

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
python_la_LIBADD = $(PYTHON_LIBS) $(GLIB_LIBS)
python_la_CPPFLAGS = $(PYTHON_CPPFLAGS)
python_la_CFLAGS = $(GLIB_CFLAGS) -I$(top_srcdir)/src/common

