## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libxchatcommon.a

INCLUDES = $(COMMON_CFLAGS)

EXTRA_DIST = makefile.msc \
	cfgfiles.h \
	ctcp.h \
	dcc.h \
	fe.h \
	history.h \
	identd.c \
	ignore.h \
	inbound.h \
	inet.h \
	jcode.c \
	jcode.h \
	make-te.c \
	modes.h \
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
	url.h \
	userlist.h \
	util.h \
	xchat.h \
	xchatc.h \
	xchat-plugin.h

if USE_OPENSSL
ssl_c = ssl.c
endif

if DO_ICONV
jcode_c = jcode.c
endif

libxchatcommon_a_SOURCES = cfgfiles.c ctcp.c dcc.c history.c ignore.c \
	inbound.c $(jcode_c) modes.c network.c notify.c outbound.c \
	plugin.c plugin-timer.c proto-irc.c server.c servlist.c $(ssl_c) \
	text.c url.c userlist.c util.c xchat.c

textevents: make-te
	./make-te < textevents.in > textevents.h 2> textenums.h

plugin.o: $(srcdir)/plugin.c
	$(COMPILE) -DXCHATLIBDIR=\"$(xchatlibdir)\" -c $(srcdir)/plugin.c
