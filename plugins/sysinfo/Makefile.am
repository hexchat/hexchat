libdir = $(hexchatlibdir)

sources = sysinfo.c format.c shared/df.c

if PLATFORM_OSX
sources += osx/backend.m
else
sources += unix/backend.c unix/match.c unix/parse.c unix/pci.c
endif

EXTRA_DIST = osx unix win32 shared format.h sysinfo.h sysinfo-backend.h

lib_LTLIBRARIES = sysinfo.la
sysinfo_la_SOURCES = $(sources)
sysinfo_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
sysinfo_la_LIBADD = $(LIBPCI_LIBS) $(GLIB_LIBS)
AM_CPPFLAGS = -I$(top_srcdir)/src/common -I$(srcdir)/shared $(LIBPCI_CFLAGS) $(GLIB_CFLAGS)
