
EXTRA_DIST=generate_header lib/HexChat.pm lib/Xchat.pm lib/HexChat/Embed.pm lib/HexChat/List/Network.pm \
	lib/HexChat/List/Network/Entry.pm lib/HexChat/List/Network/AutoJoin.pm lib/IRC.pm

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = perl.la
perl_la_SOURCES = perl.c
perl_la_LDFLAGS = -avoid-version -module 
perl_la_LIBADD = $(PERL_LDFLAGS)
BUILT_SOURCES = hexchat.pm.h irc.pm.h
#CFLAGS = @CFLAGS@ -Wno-unused
AM_CPPFLAGS = $(PERL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/../../src/common
CLEANFILES = hexchat.pm.h irc.pm.h
hexchat.pm.h irc.pm.h: lib/HexChat.pm lib/Xchat.pm lib/HexChat/Embed.pm \
	lib/HexChat/List/Network.pm lib/HexChat/List/Network/Entry.pm \
	lib/HexChat/List/Network/AutoJoin.pm lib/IRC.pm
	cd $(srcdir); perl generate_header
