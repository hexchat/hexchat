icon_DATA = hexchat.png hexchat-tray-message.png hexchat-tray-hilight.png hexchat-tray-fileoffer.png
icondir = $(datadir)/icons/hicolor/48x48/apps

hicolor_DATA = hexchat.svg
hicolordir = $(datadir)/icons/hicolor/scalable/apps

monolight_DATA = ubuntu-mono-light/hexchat-tray.svg
monolightdir = $(datadir)/icons/ubuntu-mono-light/scalable/apps

monodark_DATA = ubuntu-mono-dark/hexchat-tray.svg
monodarkdir = $(datadir)/icons/ubuntu-mono-dark/scalable/apps

UPDATE_ICON_CACHE = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor && \
					gtk-update-icon-cache -f -t $(datadir)/icons/ubuntu-mono-dark && \
					gtk-update-icon-cache -f -t $(datadir)/icons/ubuntu-mono-light || :

install-data-hook:
	$(UPDATE_ICON_CACHE);

uninstall-hook:
	$(UPDATE_ICON_CACHE);
