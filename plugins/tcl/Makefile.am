
EXTRA_DIST=makefile.msc printevents.h tclplugin.h

libdir = $(xchatlibdir)/plugins

lib_LTLIBRARIES = tcl.la
tcl_la_SOURCES = tclplugin.c
tcl_la_LDFLAGS = -avoid-version -module 
tcl_la_LIBADD = $(TCL_LIBS)
INCLUDES = $(TCL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/..
