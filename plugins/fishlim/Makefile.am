EXTRA_DIST = INSTALL LICENSE README

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = fishlim.la
fishlim_la_SOURCES = fish.c irc.c keystore.c misc.c plugin_hexchat.c
fishlim_la_LDFLAGS = -avoid-version -module 
fishlim_la_LIBADD = 
AM_CPPFLAGS = $(COMMON_CFLAGS) -I$(srcdir)/../../src/common
