localedir = $(datadir)/locale

EXTRA_DIST =				\
	remote-object.xml		\
	manager-object.xml		\
	apps_xchat_url_handler.schemas	\
	marshallers.list		\
	example.py			\
	README

BUILT_SOURCES =				\
	marshallers.h			\
	remote-object-glue.h		\
	manager-object-glue.h

CLEANFILES = $(BUILT_SOURCES)

INCLUDES =					\
	$(COMMON_CFLAGS)			\
	-DLOCALEDIR=\"$(localedir)\"

noinst_PROGRAMS = example
example_SOURCES = example.c 
example_LDADD = $(DBUS_LIBS) $(GLIB_LIBS)

manager-object-glue.h: manager-object.xml
	$(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=manager_object --mode=glib-server --output=$@ $<

remote-object-glue.h: remote-object.xml
	$(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=remote_object --mode=glib-server --output=$@ $<

marshallers.h: marshallers.list
	$(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL)  --header --body $< > $@

# Dbus service file
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.xchat.service.service.in
service_DATA = $(service_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

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
