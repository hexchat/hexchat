AppName=XChat-WDK Spelling Dictionaries
AppVerName=XChat-WDK Spelling Dictionaries 1.0
AppVersion=1.0
VersionInfoVersion=1.0
OutputBaseFilename=XChat-WDK Spelling Dictionaries
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://code.google.com/p/xchat-wdk/wiki/InfoXChat
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK Spelling Dictionaries
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\tmp-spell
OutputDir=..\build
FlatComponentsList=no
PrivilegesRequired=none
ShowComponentSizes=no
CreateUninstallRegKey=not IsTaskSelected('portable')
Uninstallable=not IsTaskSelected('portable')
DirExistsWarning=no
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64

[Tasks]
Name: portable; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked

[Files]
Source: "share\myspell\*"; DestDir: "{app}\share\myspell"; Flags: createallsubdirs recursesubdirs

[Messages]
BeveledLabel= XChat-WDK
