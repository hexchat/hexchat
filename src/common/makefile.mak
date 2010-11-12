include "..\makeinc.mak"

COMMON_OBJECTS = \
cfgfiles.obj \
chanopt.obj \
ctcp.obj \
dcc.obj \
history.obj \
ignore.obj \
inbound.obj \
modes.obj \
network.obj \
notify.obj \
outbound.obj \
plugin.obj \
plugin-timer.obj \
portable.obj \
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

all: $(COMMON_OBJECTS) xchatcommon.lib

xchatcommon.lib: $(COMMON_OBJECTS)
	lib /nologo /out:xchatcommon.lib $(COMMON_OBJECTS)

.c.obj:
	$(CC) $(CFLAGS) $(GLIB) $<

clean:
	@del *.obj
	@del xchatcommon.lib
