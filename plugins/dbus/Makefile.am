EXTRA_DIST=dbus-plugin.xml
CLEANFILES = $(BUILT_SOURCES)
INCLUDES = $(DBUS_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/.. -DLOCALEDIR=\"$(localedir)\"
BUILT_SOURCES = dbus-plugin-glue.h

libdir = $(xchatlibdir)/plugins
localedir = $(datadir)/locale

lib_LTLIBRARIES = dbus.la
dbus_la_SOURCES = dbus-plugin.c
dbus_la_LDFLAGS = -avoid-version -module 
dbus_la_LIBADD = $(DBUS_LIBS)

dbus-plugin-glue.h: dbus-plugin.xml
	libtool --mode=execute dbus-binding-tool --prefix=remote_object --mode=glib-server --output=dbus-plugin-glue.h dbus-plugin.xml

bin_PROGRAMS = xchat-remote
xchat_remote_LDADD = $(DBUS_LIBS)
xchat_remote_SOURCES = xchat-remote.c

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = url_handler.schemas

install-data-local:
	if test -z "$(DESTDIR)" && test x"$(GCONFTOOL)" != xno; then						\
		for p in $(schema_DATA) ; do					\
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p;	\
		done								\
	fi

