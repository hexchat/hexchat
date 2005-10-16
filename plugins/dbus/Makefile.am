EXTRA_DIST=dbus-plugin.xml
libdir = $(xchatlibdir)/plugins

lib_LTLIBRARIES = dbus.la
dbus_la_SOURCES = dbus-plugin.c
dbus_la_LDFLAGS = -avoid-version -module 
dbus_la_LIBADD = $(DBUS_LIBS)
INCLUDES = $(DBUS_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/.. -DLOCALEDIR=\"$(localedir)\"

BUILT_SOURCES = dbus-plugin-glue.h

dbus-plugin-glue.h: dbus-plugin.xml
	libtool --mode=execute dbus-binding-tool --prefix=remote_object --mode=glib-server --output=dbus-plugin-glue.h dbus-plugin.xml


bin_PROGRAMS = xchat-remote
xchat_remote_LDADD = $(DBUS_LIBS)
xchat_remote_SOURCES = xchat-remote.c


CLEANFILES = $(BUILT_SOURCES)
