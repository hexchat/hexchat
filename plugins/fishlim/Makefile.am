EXTRA_DIST = INSTALL LICENSE

libdir = $(hexchatlibdir)

lib_LTLIBRARIES = fishlim.la
fishlim_la_SOURCES = fish.c irc.c keystore.c plugin_hexchat.c
fishlim_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
fishlim_la_LIBADD = $(GLIB_LIBS) $(OPENSSL_LIBS)
fishlim_la_CFLAGS = $(GLIB_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/common
