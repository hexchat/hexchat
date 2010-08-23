[Setup]
AppName=XChat-WDK
AppVerName=XChat-WDK r1459-2
AppVersion=14.59.2
VersionInfoVersion=14.59.2
OutputBaseFilename=XChat-WDK r1459-2
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://code.google.com/p/xchat-wdk/wiki/InfoXChat
SetupIconFile=c:\mozilla-build\build\xchat-wdk\xchat.ico
CreateAppDir=no
DisableWelcomePage=yes
DisableReadyPage=yes
DisableFinishedPage=yes
SolidCompression=yes
SourceDir=.
OutputDir=.
PrivilegesRequired=lowest
CreateUninstallRegKey=no
Uninstallable=no
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64

[Tasks]
Name: x86; Description: "x86"; GroupDescription: "Please select which version of XChat-WDK you want to install:"; Flags: exclusive unchecked
Name: x64; Description: "x64"; GroupDescription: "Please select which version of XChat-WDK you want to install:"; Flags: exclusive; Check: Is64BitInstallMode

[Files]
Source: "XChat-WDK r1459-2 x86.exe"; DestDir: "{tmp}"; Tasks: x86
Source: "XChat-WDK r1459-2 x64.exe"; DestDir: "{tmp}"; Tasks: x64

[Messages]
BeveledLabel= XChat-WDK

[Run]
Filename: "{tmp}\XChat-WDK r1459-2 x86.exe"; Flags: nowait; Tasks: x86
Filename: "{tmp}\XChat-WDK r1459-2 x64.exe"; Flags: nowait; Tasks: x64
