## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libhexchatcommon.a

AM_CPPFLAGS = $(COMMON_CFLAGS) -I$(top_srcdir)

EXTRA_DIST = \
	cfgfiles.h \
	chanopt.h \
	ctcp.h \
	dcc.h \
	fe.h \
	hexchat.h \
	hexchatc.h \
	hexchat-plugin.h \
	history.h \
	identd.c \
	ignore.h \
	inbound.h \
	inet.h \
	make-te.c \
	modes.h \
	msproxy.h \
	network.h \
	notify.h \
	outbound.h \
	plugin.h \
	plugin-timer.h \
	proto-irc.h \
	server.h \
	servlist.h \
	ssl.h \
	ssl.c	\
	text.h \
	textenums.h \
	textevents.h \
	textevents.in \
	tree.h \
	url.h \
	userlist.h \
	util.h

if USE_OPENSSL
ssl_c = ssl.c
endif

if USE_MSPROXY
msproxy_c = msproxy.c
endif

if USE_DBUS
dbusdir = dbus
libhexchatcommon_a_LIBADD =				\
	$(top_builddir)/src/common/dbus/dbus-*.$(OBJEXT)
endif
SUBDIRS = $(dbusdir) .

if DO_PLUGIN
include_HEADERS = hexchat-plugin.h
endif

noinst_PROGRAMS = make-te

libhexchatcommon_a_SOURCES = cfgfiles.c chanopt.c ctcp.c dcc.c hexchat.c \
	history.c ignore.c inbound.c marshal.c modes.c $(msproxy_c) network.c notify.c \
	outbound.c plugin.c plugin-timer.c proto-irc.c server.c servlist.c \
	$(ssl_c) text.c tree.c url.c userlist.c util.c
libhexchatcommon_a_CFLAGS = $(LIBPROXY_CFLAGS)

textenums.h: textevents.h

textevents.h: $(srcdir)/textevents.in make-te
	$(AM_V_GEN) $(srcdir)/make-te < $< > $@ 2> textenums.h

marshal.h: $(srcdir)/marshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_hexchat_marshal --header $< > $@

marshal.c: $(srcdir)/marshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_hexchat_marshal --body $< > $@


BUILT_SOURCES = textenums.h textevents.h marshal.c marshal.h

CLEANFILES = $(BUILT_SOURCES)
