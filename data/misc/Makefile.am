util_DATA = hexchat.desktop
utildir = $(datadir)/applications

appdata_in_files = hexchat.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdatadir = $(datadir)/appdata
@INTLTOOL_XML_RULE@

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

CLEANFILES = $(appdata_DATA)

