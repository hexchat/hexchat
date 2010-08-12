include "..\..\src\makeinc.mak"

TARGET = $(PYTHONOUTPUT)

all: $(TARGET)

python.def:
	echo EXPORTS > python.def
	echo xchat_plugin_init >> python.def
	echo xchat_plugin_deinit >> python.def
	echo xchat_plugin_get_info >> python.def

python.obj: python.c
	$(CC) $(CFLAGS) /Dusleep=_sleep /DPATH_MAX=255 python.c $(GLIB) /I.. /I$(PYTHONPATH)\INCLUDE /DPYTHON_DLL=\"$(PYTHONLIB).dll\"

$(TARGET): python.obj python.def
	$(LINK) /DLL /out:$(TARGET) $(LDFLAGS) python.obj /LIBPATH:$(PYTHONPATH)\LIBS $(PYTHONLIB).lib $(LIBS) /def:python.def

clean:
	del $(TARGET)
	del *.obj
	del python.def
	del *.lib
	del *.exp
