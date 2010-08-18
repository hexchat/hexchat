[Setup]
AppName=XChat-WDK (x64)
AppVerName=XChat WDK (x64) r1459
AppVersion=14.59
VersionInfoVersion=14.59
OutputBaseFilename=XChat-WDK r1459 x64
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://code.google.com/p/xchat-wdk/downloads/list
WizardImageFile=c:\mozilla-build\build\xchat.bmp
SetupIconFile=c:\mozilla-build\build\xchat-wdk\xchat.ico
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK (x64)
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\..\xchat-wdk-x64
OutputDir=.
FlatComponentsList=no
PrivilegesRequired=none
CreateUninstallRegKey=not IsTaskSelected('portablemode')
Uninstallable=not IsTaskSelected('portablemode')
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Types]
Name: "normal"; Description: "Normal Installation"
Name: "full"; Description: "Full Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "libs"; Description: "XChat-WDK"; Types: normal full custom; Flags: fixed
Name: "translations"; Description: "Translations"; Types: normal full custom
Name: "plugins"; Description: "Language Interfaces"; Types: full custom
Name: "plugins\lua"; Description: "Lua (experimental)"; Types: full custom
Name: "plugins\lua\luawdk"; Description: "Lua-WDK 5.1.4-2"; Types: full custom
Name: "plugins\perl"; Description: "Perl (needs ActivePerl 5.12)"; Types: full custom
Name: "plugins\python"; Description: "Python (needs ActivePython 2.6)"; Types: full custom
Name: "plugins\tcl"; Description: "Tcl (needs ActiveTcl 8.5)"; Types: full custom

[Tasks]
Name: portablemode; Description: "Portable Mode (no Registry keys written and no uninstaller created)"; Flags: unchecked

[Files]
Source: "COPYING"; DestDir: "{app}"; Components: libs
Source: "libfreetype-6.dll"; DestDir: "{app}"; Components: libs
Source: "libintl-8.dll"; DestDir: "{app}"; Components: libs
Source: "libatk-1.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libcairo-2.dll"; DestDir: "{app}"; Components: libs
Source: "libeay32.dll"; DestDir: "{app}"; Components: libs
Source: "libexpat-1.dll"; DestDir: "{app}"; Components: libs
Source: "libfontconfig-1.dll"; DestDir: "{app}"; Components: libs
Source: "libgdk_pixbuf-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgdk-win32-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgio-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libglib-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgmodule-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgobject-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgthread-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libgtk-win32-2.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libpango-1.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libpangocairo-1.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libpangoft2-1.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libpangowin32-1.0-0.dll"; DestDir: "{app}"; Components: libs
Source: "libpng14-14.dll"; DestDir: "{app}"; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Components: libs
Source: "ssleay32.dll"; DestDir: "{app}"; Components: libs
Source: "zlib1.dll"; DestDir: "{app}"; Components: libs

Source: "plugins\xcewc.dll"; DestDir: "{app}\plugins"; Components: libs
Source: "plugins\xcxdcc.dll"; DestDir: "{app}\plugins"; Components: libs

Source: "xchat.exe"; DestDir: "{app}"; Components: libs
Source: "portable-mode"; DestDir: "{app}"; Tasks: portablemode

Source: "plugins\xclua.dll"; DestDir: "{app}\plugins"; Components: plugins\lua
Source: "lua51.dll"; DestDir: "{app}"; Components: plugins\lua\luawdk
Source: "plugins\xcperl.dll"; DestDir: "{app}\plugins"; Components: plugins\perl
Source: "plugins\xcpython.dll"; DestDir: "{app}\plugins"; Components: plugins\python
Source: "plugins\xctcl.dll"; DestDir: "{app}\plugins"; Components: plugins\tcl

Source: "etc\*"; DestDir: "{app}\etc"; Flags: createallsubdirs recursesubdirs; Components: libs
Source: "lib\*"; DestDir: "{app}\lib"; Flags: createallsubdirs recursesubdirs; Components: libs

Source: "locale\*"; DestDir: "{app}\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
Source: "share\*"; DestDir: "{app}\share"; Flags: createallsubdirs recursesubdirs; Components: translations

[Icons]
Name: "{group}\XChat-WDK (x64)"; Filename: "{app}\xchat.exe"; Tasks: not portablemode
Name: "{group}\Uninstall XChat-WDK (x64)"; Filename: "{uninstallexe}"; Tasks: not portablemode

[Messages]
BeveledLabel= XChat-WDK (x64)
