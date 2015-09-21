libdir = $(hexchatlibdir)

EXTRA_DIST = parse.h pci.h xsys.h match.h

lib_LTLIBRARIES = sysinfo.la
sysinfo_la_SOURCES = match.c parse.c pci.c xsys.c
sysinfo_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
sysinfo_la_LIBADD = $(LIBPCI_LIBS) $(GLIB_LIBS)
sysinfo_la_CFLAGS = $(LIBPCI_CFLAGS) $(GLIB_CFLAGS) -I$(srcdir)/../../src/common
