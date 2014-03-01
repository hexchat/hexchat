EXTRA_DIST = 

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = -avoid-version -module 
python_la_LIBADD = $(PY_LIBS)
AM_CPPFLAGS = $(PY_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/../../src/common

