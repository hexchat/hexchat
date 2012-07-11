include "..\makeinc.mak"

TARGET = gtk2-prefs.exe

PREF_OBJECTS = \
callbacks.obj \
interface.obj \
support.obj \
win32util.obj \
main.obj

CPPFLAGS = $(CPPFLAGS) /D_STL70_ /D_STATIC_CPPLIB /EHsc /DHAVE_CONFIG_H

all: $(PREF_OBJECTS) $(TARGET)

.cpp.obj:
	$(CC) $(CPPFLAGS) $(GLIB) $(GTK) /I. /c $<
	
$(TARGET): $(PREF_OBJECTS)
	$(LINK) /out:$(TARGET) /entry:mainCRTStartup $(LDFLAGS) $(PREF_OBJECTS) ntstc_msvcrt.lib $(LIBS)

clean:
	del $(TARGET)
	del *.obj
