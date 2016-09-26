
EXTRA_DIST=generate_header lib/HexChat.pm lib/Xchat.pm lib/HexChat/Embed.pm lib/HexChat/List/Network.pm \
	lib/HexChat/List/Network/Entry.pm lib/HexChat/List/Network/AutoJoin.pm lib/IRC.pm

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = perl.la
perl_la_SOURCES = perl.c
perl_la_LDFLAGS = $(PERL_LDFLAGS) $(PLUGIN_LDFLAGS) -module
perl_la_LIBADD = $(GLIB_LIBS)
perl_la_CPPFLAGS = -I$(top_srcdir)/src/common
perl_la_CFLAGS = $(PERL_CFLAGS) $(GLIB_CFLAGS)

BUILT_SOURCES = hexchat.pm.h irc.pm.h
CLEANFILES = $(BUILT_SOURCES)

hexchat.pm.h irc.pm.h: lib/HexChat.pm lib/Xchat.pm lib/HexChat/Embed.pm \
	lib/HexChat/List/Network.pm lib/HexChat/List/Network/Entry.pm \
	lib/HexChat/List/Network/AutoJoin.pm lib/IRC.pm
	cd $(srcdir); perl generate_header
