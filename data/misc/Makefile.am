util_DATA = hexchat.desktop
utildir = $(datadir)/applications

appdata_DATA = hexchat.appdata.xml
appdatadir = $(datadir)/appdata


if WITH_TM
util_DATA += htm.desktop
mime_DATA = htm-mime.xml
mimedir = $(datadir)/mime/packages

UPDATE_MIME_DATABASE = update-mime-database "$(datadir)/mime" || :
UPDATE_DESKTOP_DATABASE = update-desktop-database -q "$(datadir)/applications" || :

install-data-hook:
	$(UPDATE_MIME_DATABASE);
	$(UPDATE_DESKTOP_DATABASE);

uninstall-hook:
	$(UPDATE_MIME_DATABASE);
	$(UPDATE_DESKTOP_DATABASE);
endif
