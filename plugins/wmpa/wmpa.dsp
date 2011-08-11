# Microsoft Developer Studio Project File - Name="wmpa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=wmpa - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmpa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmpa.mak" CFG="wmpa - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmpa - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wmpa - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wmpa - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\Program Files\Microsoft Platform SDK\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build
InputPath=.\Release\wmpa.dll
InputName=wmpa
SOURCE="$(InputPath)"

"C:\msys\1.0\local\src\xchat\src\fe-gtk\plugins\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy /Y $(InputPath) "C:\msys\1.0\local\src\xchat\src\fe-gtk\plugins\$(InputName).dll"

# End Custom Build

!ELSEIF  "$(CFG)" == "wmpa - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "C:\Program Files\Microsoft Platform SDK\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
InputPath=.\Debug\wmpa.dll
InputName=wmpa
SOURCE="$(InputPath)"

"C:\msys\1.0\local\src\xchat\src\fe-gtk\plugins\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy /Y $(InputPath) "C:\msys\1.0\local\src\xchat\src\fe-gtk\plugins\$(InputName).dll"

# End Custom Build

!ENDIF 

# Begin Target

# Name "wmpa - Win32 Release"
# Name "wmpa - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\wmpa.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpa.def
# End Source File
# Begin Source File

SOURCE=.\wmpa.odl
# End Source File
# Begin Source File

SOURCE=.\wmpa.rc
# End Source File
# Begin Source File

SOURCE=.\WMPADialog.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpcdrom.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpcdromcollection.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpclosedcaption.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpcontrols.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpdvd.cpp
# End Source File
# Begin Source File

SOURCE=.\wmperror.cpp
# End Source File
# Begin Source File

SOURCE=.\wmperroritem.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpmedia.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpmediacollection.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpnetwork.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpplayer4.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpplayerapplication.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpplaylist.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpplaylistarray.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpplaylistcollection.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpsettings.cpp
# End Source File
# Begin Source File

SOURCE=.\wmpstringcollection.cpp
# End Source File
# Begin Source File

SOURCE=".\xchat-plugin.cpp"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\wmpa.h
# End Source File
# Begin Source File

SOURCE=.\WMPADIALOG.h
# End Source File
# Begin Source File

SOURCE=.\wmpcdrom.h
# End Source File
# Begin Source File

SOURCE=.\wmpcdromcollection.h
# End Source File
# Begin Source File

SOURCE=.\wmpclosedcaption.h
# End Source File
# Begin Source File

SOURCE=.\wmpcontrols.h
# End Source File
# Begin Source File

SOURCE=.\wmpdvd.h
# End Source File
# Begin Source File

SOURCE=.\wmperror.h
# End Source File
# Begin Source File

SOURCE=.\wmperroritem.h
# End Source File
# Begin Source File

SOURCE=.\wmpmedia.h
# End Source File
# Begin Source File

SOURCE=.\wmpmediacollection.h
# End Source File
# Begin Source File

SOURCE=.\wmpnetwork.h
# End Source File
# Begin Source File

SOURCE=.\wmpplayer4.h
# End Source File
# Begin Source File

SOURCE=.\wmpplayerapplication.h
# End Source File
# Begin Source File

SOURCE=.\wmpplaylist.h
# End Source File
# Begin Source File

SOURCE=.\wmpplaylistarray.h
# End Source File
# Begin Source File

SOURCE=.\wmpplaylistcollection.h
# End Source File
# Begin Source File

SOURCE=.\wmpsettings.h
# End Source File
# Begin Source File

SOURCE=.\wmpstringcollection.h
# End Source File
# Begin Source File

SOURCE=".\xchat-plugin.h"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\wmpa.rc2
# End Source File
# Begin Source File

SOURCE=.\xchat.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
# Section wmpa : {10A13217-23A7-439B-B1C0-D847C79B7774}
# 	2:5:Class:CWMPPlaylistCollection
# 	2:10:HeaderFile:wmpplaylistcollection.h
# 	2:8:ImplFile:wmpplaylistcollection.cpp
# End Section
# Section wmpa : {EC21B779-EDEF-462D-BBA4-AD9DDE2B29A7}
# 	2:5:Class:CWMPNetwork
# 	2:10:HeaderFile:wmpnetwork.h
# 	2:8:ImplFile:wmpnetwork.cpp
# End Section
# Section wmpa : {4A976298-8C0D-11D3-B389-00C04F68574B}
# 	2:5:Class:CWMPStringCollection
# 	2:10:HeaderFile:wmpstringcollection.h
# 	2:8:ImplFile:wmpstringcollection.cpp
# End Section
# Section wmpa : {B7ABC220-DF71-11CF-8E74-00A0C90F26F8}
# 	2:5:Class:Cmci
# 	2:10:HeaderFile:mci.h
# 	2:8:ImplFile:mci.cpp
# End Section
# Section wmpa : {8363BC22-B4B4-4B19-989D-1CD765749DD1}
# 	2:5:Class:CWMPMediaCollection
# 	2:10:HeaderFile:wmpmediacollection.h
# 	2:8:ImplFile:wmpmediacollection.cpp
# End Section
# Section wmpa : {679409C0-99F7-11D3-9FB7-00105AA620BB}
# 	2:5:Class:CWMPPlaylistArray
# 	2:10:HeaderFile:wmpplaylistarray.h
# 	2:8:ImplFile:wmpplaylistarray.cpp
# End Section
# Section wmpa : {74C09E02-F828-11D2-A74B-00A0C905F36E}
# 	2:5:Class:CWMPControls
# 	2:10:HeaderFile:wmpcontrols.h
# 	2:8:ImplFile:wmpcontrols.cpp
# End Section
# Section wmpa : {CFAB6E98-8730-11D3-B388-00C04F68574B}
# 	2:5:Class:CWMPCdrom
# 	2:10:HeaderFile:wmpcdrom.h
# 	2:8:ImplFile:wmpcdrom.cpp
# End Section
# Section wmpa : {8DA61686-4668-4A5C-AE5D-803193293DBE}
# 	2:5:Class:CWMPDVD
# 	2:10:HeaderFile:wmpdvd.h
# 	2:8:ImplFile:wmpdvd.cpp
# End Section
# Section wmpa : {6C497D62-8919-413C-82DB-E935FB3EC584}
# 	2:5:Class:CWMPPlayer4
# 	2:10:HeaderFile:wmpplayer4.h
# 	2:8:ImplFile:wmpplayer4.cpp
# End Section
# Section wmpa : {40897764-CEAB-47BE-AD4A-8E28537F9BBF}
# 	2:5:Class:CWMPPlayerApplication
# 	2:10:HeaderFile:wmpplayerapplication.h
# 	2:8:ImplFile:wmpplayerapplication.cpp
# End Section
# Section wmpa : {6BF52A52-394A-11D3-B153-00C04F79FAA6}
# 	2:21:DefaultSinkHeaderFile:wmpplayer4.h
# 	2:16:DefaultSinkClass:CWMPPlayer4
# End Section
# Section wmpa : {C1A8AF25-1257-101B-8FB0-0020AF039CA3}
# 	2:21:DefaultSinkHeaderFile:mci.h
# 	2:16:DefaultSinkClass:Cmci
# End Section
# Section wmpa : {9104D1AB-80C9-4FED-ABF0-2E6417A6DF14}
# 	2:5:Class:CWMPSettings
# 	2:10:HeaderFile:wmpsettings.h
# 	2:8:ImplFile:wmpsettings.cpp
# End Section
# Section wmpa : {A12DCF7D-14AB-4C1B-A8CD-63909F06025B}
# 	2:5:Class:CWMPError
# 	2:10:HeaderFile:wmperror.h
# 	2:8:ImplFile:wmperror.cpp
# End Section
# Section wmpa : {7BF80981-BF32-101A-8BBB-00AA00300CAB}
# 	2:5:Class:CPicture
# 	2:10:HeaderFile:picture.h
# 	2:8:ImplFile:picture.cpp
# End Section
# Section wmpa : {D5F0F4F1-130C-11D3-B14E-00C04F79FAA6}
# 	2:5:Class:CWMPPlaylist
# 	2:10:HeaderFile:wmpplaylist.h
# 	2:8:ImplFile:wmpplaylist.cpp
# End Section
# Section wmpa : {EE4C8FE2-34B2-11D3-A3BF-006097C9B344}
# 	2:5:Class:CWMPCdromCollection
# 	2:10:HeaderFile:wmpcdromcollection.h
# 	2:8:ImplFile:wmpcdromcollection.cpp
# End Section
# Section wmpa : {4F2DF574-C588-11D3-9ED0-00C04FB6E937}
# 	2:5:Class:CWMPClosedCaption
# 	2:10:HeaderFile:wmpclosedcaption.h
# 	2:8:ImplFile:wmpclosedcaption.cpp
# End Section
# Section wmpa : {94D55E95-3FAC-11D3-B155-00C04F79FAA6}
# 	2:5:Class:CWMPMedia
# 	2:10:HeaderFile:wmpmedia.h
# 	2:8:ImplFile:wmpmedia.cpp
# End Section
# Section wmpa : {3614C646-3B3B-4DE7-A81E-930E3F2127B3}
# 	2:5:Class:CWMPErrorItem
# 	2:10:HeaderFile:wmperroritem.h
# 	2:8:ImplFile:wmperroritem.cpp
# End Section
