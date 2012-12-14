## Process this file with automake to produce Makefile.in

SUBDIRS = po intl src plugins man

EXTRA_DIST = autogen.sh

icon_DATA = src/pixmaps/hexchat.png
icondir = $(datadir)/pixmaps

util_DATA = share/misc/hexchat.desktop
utildir = $(datadir)/applications

hicolor_DATA = share/icons/hexchat.svg
hicolordir = $(datadir)/icons/hicolor/scalable/apps

UPDATE_ICON_CACHE = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	$(UPDATE_ICON_CACHE);

uninstall-hook:
	$(UPDATE_ICON_CACHE);

ACLOCAL_AMFLAGS = -I m4
