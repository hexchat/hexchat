
libdir = $(xchatlibdir)/plugins

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = -avoid-version -module 
python_la_LIBADD = $(PY_LIBS)
INCLUDES = $(PY_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/..

