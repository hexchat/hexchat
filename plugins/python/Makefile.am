AM_CFLAGS = @hexchat_def_CFLAGS@
AM_LDFLAGS = @hexchat_def_LDFLAGS@

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
python_la_LIBADD = $(PYTHON_LIBS) $(GLIB_LIBS) @hexchat_def_LDFLAGS@
python_la_CPPFLAGS = -I$(top_srcdir)/src/common $(PYTHON_CPPFLAGS)
python_la_CFLAGS = $(GLIB_CFLAGS) @hexchat_def_CFLAGS@

