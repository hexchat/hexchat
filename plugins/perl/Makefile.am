
EXTRA_DIST=alt_completion.pl ignore.pl sample_script.pl xchat2-perldocs.html xchat2-perl.html generate_header Xchat.pm IRC.pm makefile.msc

libdir = $(xchatlibdir)/plugins

lib_LTLIBRARIES = perl.la
perl_la_SOURCES = perl.c
perl_la_LDFLAGS = -avoid-version -module 
perl_la_LIBADD = $(PERL_LDFLAGS)
BUILT_SOURCES = xchat.pm.h
#CFLAGS = @CFLAGS@ -Wno-unused
INCLUDES = $(PERL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/..
CLEANFILES = xchat.pm.h
xchat.pm.h: Xchat.pm IRC.pm
	perl generate_header
