include "..\..\src\makeinc.mak"

TARGET = xcwmpa.dll

WMPA_OBJECTS = \
wmpa.obj \
wmpadialog.obj \
wmpcdrom.obj \
wmpcdromcollection.obj \
wmpclosedcaption.obj \
wmpcontrols.obj \
wmpdvd.obj \
wmperror.obj \
wmperroritem.obj \
wmpmedia.obj \
wmpmediacollection.obj \
wmpnetwork.obj \
wmpplayer4.obj \
wmpplayerapplication.obj \
wmpplaylist.obj \
wmpplaylistarray.obj \
wmpplaylistcollection.obj \
wmpsettings.obj \
wmpstringcollection.obj \
xchat-plugin.obj

CPPFLAGS = $(CPPFLAGS) /EHsc /D_AFXDLL /D_AFX_NO_DAO_SUPPORT /D_WINDLL /D_USRDLL 

all: $(WMPA_OBJECTS) $(TARGET)

.cpp.obj:
	$(CC) $(CPPFLAGS) /Yc"StdAfx.h" /Fp"wmpa.pch" StdAfx.cpp
	$(CC) $(CPPFLAGS) /Yu"StdAfx.h" /Fp"wmpa.pch" /c $<

$(TARGET): $(WMPA_OBJECTS)
	rc /nologo /D_AFXDLL wmpa.rc
!ifdef X64
	midl /nologo /mktyplib203 /char signed /env x64 /h wmpa_h.h /tlb wmpa.tlb wmpa.odl
!else
	midl /nologo /mktyplib203 /char signed /env win32 /h wmpa_h.h /tlb wmpa.tlb wmpa.odl
!endif
	$(LINK) /DLL /out:$(TARGET) $(LDFLAGS) $(WMPA_OBJECTS) $(LIBS) /def:wmpa.def wmpa.res

clean:
	del $(TARGET)
	del *.obj
	del wmpa.pch
	del wmpa.res
	del wmpa.tlb
	del wmpa_h.h
	del wmpa_i.c
	del *.exp
	del *.lib
