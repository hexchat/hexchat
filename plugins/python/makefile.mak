include "..\..\src\makeinc.mak"

DIRENTLIB = ..\..\src\common\dirent-win32.lib
TARGET = $(PYTHONOUTPUT)

all: $(TARGET)

python.def:
	echo EXPORTS > python.def
	echo xchat_plugin_init >> python.def
	echo xchat_plugin_deinit >> python.def
	echo xchat_plugin_get_info >> python.def

python.obj: python.c
	$(CC) $(CFLAGS) /I.. /Dusleep=_sleep /DPATH_MAX=255 python.c $(GLIB) /I$(PYTHONPATH)\include /DPYTHON_DLL=\"$(PYTHONLIB).dll\"

$(TARGET): python.obj python.def
	$(LINK) /dll /out:$(TARGET) $(LDFLAGS) python.obj /libpath:$(PYTHONPATH)\libs $(PYTHONLIB).lib $(DIRENTLIB) $(LIBS) /def:python.def

clean:
	del $(TARGET)
	del *.obj
	del python.def
	del *.lib
	del *.exp
