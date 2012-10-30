
EXTRA_DIST=printevents.h tclplugin.h README

libdir = $(hexchatlibdir)/plugins

lib_LTLIBRARIES = tcl.la
tcl_la_SOURCES = tclplugin.c
tcl_la_LDFLAGS = -avoid-version -module 
tcl_la_LIBADD = $(TCL_LIBS)
AM_CPPFLAGS = $(TCL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/../../src/common
