icon_DATA = hexchat.png
icondir = $(datadir)/icons/hicolor/48x48/apps

hicolor_DATA = hexchat.svg
hicolordir = $(datadir)/icons/hicolor/scalable/apps

UPDATE_ICON_CACHE = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor || :

install-data-hook:
	$(UPDATE_ICON_CACHE);

uninstall-hook:
	$(UPDATE_ICON_CACHE);

EXTRA_DIST = $(icon_DATA) $(hicolor_DATA)
