AppName=XChat-WDK (x64)
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://www.xchat-wdk.org/home/downloads
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK (x64)
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK (x64)
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\dist-x64
OutputDir=..\build
FlatComponentsList=no
PrivilegesRequired=none
ShowComponentSizes=no
CreateUninstallRegKey=not IsTaskSelected('portable')
Uninstallable=not IsTaskSelected('portable')
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64

[Types]
Name: "normal"; Description: "Normal Installation"
Name: "full"; Description: "Full Installation"
Name: "minimal"; Description: "Minimal Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "libs"; Description: "XChat-WDK"; Types: normal full minimal custom; Flags: fixed
Name: "xctext"; Description: "XChat-Text"; Types: full custom; Flags: disablenouninstallwarning
Name: "translations"; Description: "Translations"; Types: normal full custom; Flags: disablenouninstallwarning
;Name: "spelling"; Description: "Spelling Dictionaries"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins"; Description: "Plugins"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\checksum"; Description: "Checksum"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\doat"; Description: "Do At"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\mpcinfo"; Description: "mpcInfo"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\upd"; Description: "Update Checker"; Types: normal full custom; Flags: disablenouninstallwarning
Name: "plugins\winamp"; Description: "Winamp"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\wmpa"; Description: "Windows Media Player Announcer"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\xtray"; Description: "X-Tray"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs"; Description: "Language Interfaces"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\lua"; Description: "Lua"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\lua\luawdk"; Description: "Lua-WDK"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\perl"; Description: "Perl"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\python"; Description: "Python"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\tcl"; Description: "Tcl"; Types: full custom; Flags: disablenouninstallwarning

[Tasks]
Name: portable; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked

Name: perl512; Description: "5.12"; GroupDescription: "Perl version:"; Flags: exclusive; Components: langs\perl
Name: perl514; Description: "5.14"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl

[Registry]
Root: HKCR; Subkey: "irc"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey uninsdeletekey; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: ""; ValueData: "URL:IRC Protocol"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\xchat.exe,0"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell\open\command"; ValueType: string; ValueName: ""; ValueData: "{app}\xchat.exe --url=""%1"""; Flags: uninsdeletevalue; Tasks: not portable

[Run]
Filename: "{app}\xchat.exe"; Description: "Run XChat-WDK after closing the Wizard"; Flags: nowait postinstall skipifsilent

[Files]
Source: "portable-mode"; DestDir: "{app}"; Tasks: portable

Source: "cert.pem"; DestDir: "{app}"; Components: libs
Source: "COPYING"; DestDir: "{app}"; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Components: libs
Source: "LICENSE.GTK"; DestDir: "{app}"; Components: libs
Source: "LICENSE.CAIRO"; DestDir: "{app}"; Components: libs
Source: "LICENSE.LUA"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ENCHANT"; DestDir: "{app}"; Components: libs
Source: "LICENSE.LIBXML"; DestDir: "{app}"; Components: libs
Source: "etc\*"; DestDir: "{app}\etc"; Flags: createallsubdirs recursesubdirs; Components: libs
Source: "share\xml\*"; DestDir: "{app}\share\xml"; Flags: createallsubdirs recursesubdirs; Components: libs
Source: "locale\*"; DestDir: "{app}\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
Source: "share\locale\*"; DestDir: "{app}\share\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
;Source: "share\myspell\*"; DestDir: "{app}\share\myspell"; Flags: createallsubdirs recursesubdirs; Components: spelling

Source: "freetype6.dll"; DestDir: "{app}"; Components: libs
Source: "intl.dll"; DestDir: "{app}"; Components: libs

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
Source: "lua51.dll"; DestDir: "{app}"; Components: libs
Source: "ssleay32.dll"; DestDir: "{app}"; Components: libs
Source: "zlib1.dll"; DestDir: "{app}"; Components: libs
Source: "libxml2.dll"; DestDir: "{app}"; Components: libs
Source: "libenchant.dll"; DestDir: "{app}"; Components: libs

Source: "lib\enchant\libenchant_myspell.dll"; DestDir: "{app}\lib\enchant"; Components: libs

Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs
Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs
Source: "lib\gtk-2.0\modules\libgail.dll"; DestDir: "{app}\lib\gtk-2.0\modules"; Components: libs

Source: "plugins\xcchecksum.dll"; DestDir: "{app}\plugins"; Components: plugins\checksum
Source: "plugins\xcdoat.dll"; DestDir: "{app}\plugins"; Components: plugins\doat
Source: "plugins\xcmpcinfo.dll"; DestDir: "{app}\plugins"; Components: plugins\mpcinfo
Source: "plugins\xcupd.dll"; DestDir: "{app}\plugins"; Components: plugins\upd
Source: "plugins\xcwinamp.dll"; DestDir: "{app}\plugins"; Components: plugins\winamp
Source: "plugins\xtray.dll"; DestDir: "{app}\plugins"; Components: plugins\xtray
Source: "plugins\xcwmpa.dll"; DestDir: "{app}\plugins"; Components: plugins\wmpa

Source: "plugins\xclua.dll"; DestDir: "{app}\plugins"; Components: langs\lua
Source: "plugins\xcpython.dll"; DestDir: "{app}\plugins"; Components: langs\python
Source: "plugins\xctcl.dll"; DestDir: "{app}\plugins"; Components: langs\tcl

Source: "plugins\xcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: perl512
Source: "plugins\xcperl-514.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: perl514

Source: "xchat.exe"; DestDir: "{app}"; Components: libs
Source: "xchat-text.exe"; DestDir: "{app}"; Components: xctext

[Icons]
Name: "{group}\XChat-WDK (x64)"; Filename: "{app}\xchat.exe"; Tasks: not portable
Name: "{group}\XChat-Text (x64)"; Filename: "{app}\xchat-text.exe"; Components: xctext; Tasks: not portable
Name: "{group}\Uninstall XChat-WDK (x64)"; Filename: "{uninstallexe}"; Tasks: not portable

[Messages]
BeveledLabel= XChat-WDK

[Code]
/////////////////////////////////////////////////////////////////////
// these are required for x86->x64 or reverse upgrades
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
	if not (IsTaskSelected('portable')) then
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
