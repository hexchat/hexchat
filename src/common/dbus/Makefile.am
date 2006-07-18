localedir = $(datadir)/locale

EXTRA_DIST =				\
	dbus-plugin.xml			\
	apps_xchat_url_handler.schemas	\
	marshallers.list		\
	example.py			\
	README

BUILT_SOURCES =				\
	marshallers.h			\
	dbus-plugin-glue.h

CLEANFILES = $(BUILT_SOURCES)

INCLUDES =					\
	$(COMMON_CFLAGS)			\
	-DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = xchat-remote
xchat_remote_SOURCES = xchat-remote.c 
xchat_remote_LDADD = $(DBUS_LIBS) $(GLIB_LIBS)

noinst_PROGRAMS = example
example_SOURCES = example.c 
example_LDADD = $(DBUS_LIBS) $(GLIB_LIBS)

dbus-plugin-glue.h: dbus-plugin.xml
	$(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=remote_object --mode=glib-server --output=$@ $<

marshallers.h: marshallers.list
	$(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL)  --header --body $< > $@

if DO_GCONF
GCONF_SCHEMA_CONFIG_SOURCE = `$(GCONFTOOL) --get-default-source`
GCONF_SCHEMA_FILE_DIR = $(sysconfdir)/gconf/schemas
schemadir = $(GCONF_SCHEMA_FILE_DIR)
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
