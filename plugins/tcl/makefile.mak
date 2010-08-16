include "..\..\src\makeinc.mak"

TARGET = $(TCLOUTPUT)

all: $(TARGET)

tcl.def:
	echo EXPORTS > tcl.def
	echo xchat_plugin_init >> tcl.def
	echo xchat_plugin_deinit >> tcl.def
	echo xchat_plugin_get_info >> tcl.def

tclplugin.obj: tclplugin.c
	$(CC) $(CFLAGS) tclplugin.c /I$(TCLPATH)\include /I../../include /I.. /DTCL_DLL=\"$(TCLLIB).dll\"

$(TARGET): tclplugin.obj tcl.def
	$(LINK) /DLL /out:$(TARGET) $(LDFLAGS) tclplugin.obj /libpath:$(TCLPATH)\lib $(TCLLIB).lib /DELAYLOAD:$(TCLLIB).dll DELAYIMP.LIB /def:tcl.def

clean:
	del $(TARGET)
	del *.obj
	del tcl.def
