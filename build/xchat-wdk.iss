[Setup]
AppName=XChat-WDK
AppVerName=XChat-WDK r1464-2
AppVersion=14.64.2
VersionInfoVersion=14.64.2
OutputBaseFilename=XChat-WDK r1464-2
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://code.google.com/p/xchat-wdk/wiki/InfoXChat
WizardImageFile=c:\mozilla-build\build\xchat-wdk\xchat.bmp
SetupIconFile=c:\mozilla-build\build\xchat-wdk\xchat.ico
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\..\xchat-wdk-uni
OutputDir=..\xchat-wdk\build
FlatComponentsList=no
PrivilegesRequired=none
CreateUninstallRegKey=not IsTaskSelected('portablemode')
Uninstallable=not IsTaskSelected('portablemode')
ArchitecturesAllowed=x86 x64
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
Name: "plugins\perl"; Description: "Perl (needs ActivePerl)"; Types: full custom
Name: "plugins\python"; Description: "Python (needs ActivePython 2.6)"; Types: full custom
Name: "plugins\tcl"; Description: "Tcl (needs ActiveTcl 8.5)"; Types: full custom

[Tasks]
Name: portablemode; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked
Name: x86; Description: "x86"; GroupDescription: "XChat-WDK version:"; Flags: exclusive unchecked
Name: x64; Description: "x64"; GroupDescription: "XChat-WDK version:"; Flags: exclusive; Check: Is64BitInstallMode
Name: perl58; Description: "5.8"; GroupDescription: "ActivePerl version:"; Flags: exclusive unchecked
Name: perl510; Description: "5.10"; GroupDescription: "ActivePerl version:"; Flags: exclusive unchecked
Name: perl512; Description: "5.12"; GroupDescription: "ActivePerl version:"; Flags: exclusive

[Files]
Source: "COPYING"; DestDir: "{app}"; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Components: libs
Source: "portable-mode"; DestDir: "{app}"; Tasks: portablemode
Source: "etc\*"; DestDir: "{app}\etc"; Flags: createallsubdirs recursesubdirs; Components: libs
Source: "locale\*"; DestDir: "{app}\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
Source: "share\*"; DestDir: "{app}\share"; Flags: createallsubdirs recursesubdirs; Components: translations



Source: "xchat.exe"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "xchat.exe.x64"; DestDir: "{app}"; DestName: "xchat.exe"; Components: libs; Tasks: x64



Source: "freetype6.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libfreetype-6.dll.x64"; DestDir: "{app}"; DestName: "libfreetype-6.dll"; Components: libs; Tasks: x64

Source: "intl.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libintl-8.dll.x64"; DestDir: "{app}"; DestName: "libintl-8.dll"; Components: libs; Tasks: x64



Source: "libatk-1.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libatk-1.0-0.dll.x64"; DestDir: "{app}"; DestName: "libatk-1.0-0.dll"; Components: libs; Tasks: x64

Source: "libcairo-2.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libcairo-2.dll.x64"; DestDir: "{app}"; DestName: "libcairo-2.dll"; Components: libs; Tasks: x64

Source: "libeay32.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libeay32.dll.x64"; DestDir: "{app}"; DestName: "libeay32.dll"; Components: libs; Tasks: x64

Source: "libexpat-1.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libexpat-1.dll.x64"; DestDir: "{app}"; DestName: "libexpat-1.dll"; Components: libs; Tasks: x64

Source: "libfontconfig-1.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libfontconfig-1.dll.x64"; DestDir: "{app}"; DestName: "libfontconfig-1.dll"; Components: libs; Tasks: x64

Source: "libgdk_pixbuf-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgdk_pixbuf-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgdk_pixbuf-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgdk-win32-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgdk-win32-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgdk-win32-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgio-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgio-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgio-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libglib-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libglib-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libglib-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgmodule-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgmodule-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgmodule-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgobject-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgobject-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgobject-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgthread-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgthread-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgthread-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libgtk-win32-2.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libgtk-win32-2.0-0.dll.x64"; DestDir: "{app}"; DestName: "libgtk-win32-2.0-0.dll"; Components: libs; Tasks: x64

Source: "libpango-1.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libpango-1.0-0.dll.x64"; DestDir: "{app}"; DestName: "libpango-1.0-0.dll"; Components: libs; Tasks: x64

Source: "libpangocairo-1.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libpangocairo-1.0-0.dll.x64"; DestDir: "{app}"; DestName: "libpangocairo-1.0-0.dll"; Components: libs; Tasks: x64

Source: "libpangoft2-1.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libpangoft2-1.0-0.dll.x64"; DestDir: "{app}"; DestName: "libpangoft2-1.0-0.dll"; Components: libs; Tasks: x64

Source: "libpangowin32-1.0-0.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libpangowin32-1.0-0.dll.x64"; DestDir: "{app}"; DestName: "libpangowin32-1.0-0.dll"; Components: libs; Tasks: x64

Source: "libpng14-14.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libpng14-14.dll.x64"; DestDir: "{app}"; DestName: "libpng14-14.dll"; Components: libs; Tasks: x64

Source: "lua51.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "lua51.dll.x64"; DestDir: "{app}"; DestName: "lua51.dll"; Components: libs; Tasks: x64

Source: "ssleay32.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "ssleay32.dll.x64"; DestDir: "{app}"; DestName: "ssleay32.dll"; Components: libs; Tasks: x64

Source: "zlib1.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "zlib1.dll.x64"; DestDir: "{app}"; DestName: "zlib1.dll"; Components: libs; Tasks: x64



Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll.x64"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; DestName: "libpixmap.dll"; Components: libs; Tasks: x64

Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll.x64"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; DestName: "libwimp.dll"; Components: libs; Tasks: x64

Source: "lib\gtk-2.0\modules\libgail.dll"; DestDir: "{app}\lib\gtk-2.0\modules"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\modules\libgail.dll.x64"; DestDir: "{app}\lib\gtk-2.0\modules"; DestName: "libgail.dll"; Components: libs; Tasks: x64



Source: "plugins\xcewc.dll"; DestDir: "{app}\plugins"; Components: libs; Tasks: x86
Source: "plugins\xcewc.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcewc.dll"; Components: libs; Tasks: x64

Source: "plugins\xcxdcc.dll"; DestDir: "{app}\plugins"; Components: libs; Tasks: x86
Source: "plugins\xcxdcc.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcxdcc.dll"; Components: libs; Tasks: x64



Source: "plugins\xclua.dll"; DestDir: "{app}\plugins"; Components: plugins\lua; Tasks: x86
Source: "plugins\xclua.dll.x64"; DestDir: "{app}\plugins"; DestName: "xclua.dll"; Components: plugins\lua; Tasks: x64

Source: "plugins\xcpython.dll"; DestDir: "{app}\plugins"; Components: plugins\python; Tasks: x86
Source: "plugins\xcpython.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcpython.dll"; Components: plugins\python; Tasks: x64

Source: "plugins\xctcl.dll"; DestDir: "{app}\plugins"; Components: plugins\tcl; Tasks: x86
Source: "plugins\xctcl.dll.x64"; DestDir: "{app}\plugins"; DestName: "xctcl.dll"; Components: plugins\tcl; Tasks: x64



Source: "plugins\xcperl-58.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x86 and perl58
Source: "plugins\xcperl-510.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x64 and perl58

Source: "plugins\xcperl-510.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x86 and perl510
Source: "plugins\xcperl-510.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x64 and perl510

Source: "plugins\xcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x86 and perl512
Source: "plugins\xcperl-512.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: plugins\perl; Tasks: x64 and perl512

[Icons]
Name: "{group}\XChat-WDK"; Filename: "{app}\xchat.exe"; Tasks: not portablemode
Name: "{group}\Uninstall XChat-WDK"; Filename: "{uninstallexe}"; Tasks: not portablemode

[Messages]
BeveledLabel= XChat-WDK

[Code]
/////////////////////////////////////////////////////////////////////
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\XChat-WDK_is1');
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;


/////////////////////////////////////////////////////////////////////
function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;


/////////////////////////////////////////////////////////////////////
function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 1 - uninstall string is empty
// 2 - error executing the UnInstallString
// 3 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  // get the uninstall string of the old app
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

/////////////////////////////////////////////////////////////////////
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if not (IsTaskSelected('portablemode')) then
  begin
    if (CurStep=ssInstall) then
    begin
      if (IsUpgrade()) then
      begin
        UnInstallOldVersion();
      end;
    end;
  end;
end;
