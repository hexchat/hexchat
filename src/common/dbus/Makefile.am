noinst_LIBRARIES = libhexchatdbus.a
libhexchatdbus_a_SOURCES =			\
	dbus-plugin.c				\
	dbus-plugin.h				\
	dbus-client.c				\
	dbus-client.h

EXTRA_DIST =				\
	remote-object.xml		\
	example.py			\
	org.hexchat.service.service.in

BUILT_SOURCES =				\
	remote-object-glue.h

# Dbus service file
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.hexchat.service.service.in
service_DATA = $(service_in_files:.service.in=.service)

CLEANFILES = $(BUILT_SOURCES) $(service_DATA)

AM_CPPFLAGS = -I$(top_srcdir)/src/common $(COMMON_CFLAGS) $(DBUS_CFLAGS)

noinst_PROGRAMS = example
example_SOURCES = example.c
example_LDADD = $(DBUS_LIBS) $(GLIB_LIBS)

remote-object-glue.h: remote-object.xml
	$(AM_V_GEN) $(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=remote_object --mode=glib-server --output=$@ $<

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

