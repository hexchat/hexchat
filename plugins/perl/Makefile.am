
EXTRA_DIST=ignore.pl sample_script.pl xchat2-perldocs.html xchat2-perl.html makefile.msc

libdir = $(xchatlibdir)/plugins

lib_LTLIBRARIES = perl.la
perl_la_SOURCES = perl.c
perl_la_LDFLAGS = -avoid-version -module 
perl_la_LIBADD = $(PERL_LDFLAGS)
#CFLAGS = @CFLAGS@ -Wno-unused
INCLUDES = $(PERL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/..
