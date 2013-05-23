## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libhexchatcommon.a

AM_CPPFLAGS = $(COMMON_CFLAGS)

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
	profile.h \
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

if USE_DBUS
dbusdir = dbus
libhexchatcommon_a_LIBADD =				\
	$(top_builddir)/src/common/dbus/dbus-*.$(OBJEXT)
endif
SUBDIRS = $(dbusdir) .

if DO_PLUGIN
include_HEADERS = hexchat-plugin.h
endif

libhexchatcommon_a_SOURCES = cfgfiles.c chanopt.c ctcp.c dcc.c hexchat.c \
	history.c ignore.c inbound.c modes.c msproxy.c network.c notify.c \
	outbound.c plugin.c plugin-timer.c profile.c proto-irc.c server.c \
	servlist.c $(ssl_c) text.c tree.c url.c userlist.c util.c
libhexchatcommon_a_CFLAGS = $(LIBPROXY_CFLAGS)

textenums.h: textevents.h

textevents.h: textevents.in make-te
	./make-te < textevents.in > textevents.h 2> textenums.h

BUILT_SOURCES = textenums.h textevents.h
