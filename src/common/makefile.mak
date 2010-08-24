include "..\makeinc.mak"

all: $(COMMON_OBJECTS) xchatcommon.lib

xchatcommon.lib: $(COMMON_OBJECTS)
	lib /nologo /out:xchatcommon.lib $(COMMON_OBJECTS)

.c.obj:
	$(CC) $(CFLAGS) $(GLIB) $<

clean:
	del *.obj
	del xchatcommon.lib
