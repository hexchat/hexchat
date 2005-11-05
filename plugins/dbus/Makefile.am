EXTRA_DIST=dbus-plugin.xml apps_xchat_url_handler.schemas marshallers.list example.py
INCLUDES = $(DBUS_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/.. -DLOCALEDIR=\"$(localedir)\"
BUILT_SOURCES = dbus-plugin-glue.h marshallers.h
CLEANFILES = $(BUILT_SOURCES)

libdir = $(xchatlibdir)/plugins
localedir = $(datadir)/locale

lib_LTLIBRARIES = dbus.la
dbus_la_SOURCES = dbus-plugin.c
dbus_la_LDFLAGS = -avoid-version -module 
dbus_la_LIBADD = $(DBUS_LIBS)

bin_PROGRAMS = xchat-remote
xchat_remote_SOURCES = xchat-remote.c 
xchat_remote_LDADD = $(DBUS_LIBS)

dbus-plugin-glue.h: dbus-plugin.xml
	libtool --mode=execute dbus-binding-tool --prefix=remote_object --mode=glib-server --output=dbus-plugin-glue.h dbus-plugin.xml

marshallers.h: marshallers.list
	libtool --mode=execute glib-genmarshal --header --body marshallers.list > marshallers.h

if DO_GCONF
GCONF_SCHEMA_CONFIG_SOURCE=`$(GCONFTOOL) --get-default-source`
GCONF_SCHEMA_FILE_DIR=$(sysconfdir)/gconf/schemas
schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = apps_xchat_url_handler.schemas
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p; \
		done \
	fi
else
install-data-local:
endif
