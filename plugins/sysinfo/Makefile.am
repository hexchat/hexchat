libdir = $(hexchatlibdir)

lib_LTLIBRARIES = sysinfo.la
sysinfo_la_SOURCES = hwmon.c match.c parse.c pci.c xsys.c
sysinfo_la_LDFLAGS = -avoid-version -module
sysinfo_la_LIBADD = -lpci
AM_CPPFLAGS = $(COMMON_CFLAGS) -I$(srcdir)/../../src/common
