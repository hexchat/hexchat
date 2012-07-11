include "..\makeinc.mak"

COMMON_OBJECTS = \
cfgfiles.obj \
chanopt.obj \
ctcp.obj \
dcc.obj \
dirent-win32.obj \
history.obj \
identd.obj \
ignore.obj \
inbound.obj \
modes.obj \
network.obj \
notify.obj \
outbound.obj \
plugin.obj \
plugin-timer.obj \
proto-irc.obj \
server.obj \
servlist.obj \
ssl.obj \
text.obj \
thread.obj \
tree.obj \
url.obj \
userlist.obj \
util.obj \
xchat.obj

all: $(COMMON_OBJECTS) xchatcommon.lib dirent-win32.lib

xchatcommon.lib: $(COMMON_OBJECTS)
	lib /nologo /out:xchatcommon.lib $(COMMON_OBJECTS)

dirent-win32.lib: dirent-win32.obj
	lib /nologo /out:dirent-win32.lib dirent-win32.obj

.c.obj::
	$(CC) $(CFLAGS) $(GLIB) $<

clean:
	@del *.obj
	@del xchatcommon.lib
	@del dirent-win32.lib
