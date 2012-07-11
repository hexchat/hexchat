include "..\..\src\makeinc.mak"

TARGET = xtray.dll

TRAY_OBJECTS = \
callbacks.obj \
sdAlerts.obj \
sdTray.obj \
utility.obj \
xchat.obj \
xtray.obj

CPPFLAGS = $(CPPFLAGS) /D_STL70_ /D_STATIC_CPPLIB /EHsc /DUNICODE /D_UNICODE

all: $(TRAY_OBJECTS) $(TARGET)

xtray.def:
	echo EXPORTS > xtray.def
	echo xchat_plugin_init >> xtray.def
	echo xchat_plugin_deinit >> xtray.def

.cpp.obj:
	$(CC) $(CPPFLAGS) /I.. /c $<

res:
	rc /nologo resource.rc
	
$(TARGET): $(TRAY_OBJECTS) xtray.def res
	$(LINK) /DLL /out:$(TARGET) $(LDFLAGS) $(TRAY_OBJECTS) ntstc_msvcrt.lib $(LIBS) /def:xtray.def resource.res

clean:
	del $(TARGET)
	del *.obj
	del xtray.def
	del resource.res
	del *.lib
	del *.exp
