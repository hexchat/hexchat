SUBDIRS =

if DO_PLUGIN
SUBDIRS += pkgconfig
endif

if DO_GTK
SUBDIRS += icons misc man
else
if WITH_TM
SUBDIRS += misc
endif
endif

EXTRA_DIST = \
	hexchat.gresource.xml
