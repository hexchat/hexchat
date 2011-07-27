AppName=XChat-WDK
AppVerName=XChat-WDK 1496-3
AppVersion=14.96.3
VersionInfoVersion=14.96.3
OutputBaseFilename=XChat-WDK 1496-3
AppPublisher=XChat-WDK
AppPublisherURL=http://code.google.com/p/xchat-wdk/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://code.google.com/p/xchat-wdk/wiki/InfoXChat
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\tmp
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
;Name: "plugins\ewc"; Description: "EasyWinampControl"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\checksum"; Description: "Checksum"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\mpcinfo"; Description: "mpcInfo"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\upd"; Description: "Update Checker"; Types: normal full custom; Flags: disablenouninstallwarning
Name: "plugins\winamp"; Description: "Winamp"; Types: full custom; Flags: disablenouninstallwarning
;Name: "plugins\xdcc"; Description: "XDCC"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\xtray"; Description: "X-Tray"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs"; Description: "Language Interfaces"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\lua"; Description: "Lua"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\lua\luawdk"; Description: "Lua-WDK"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\perl"; Description: "Perl"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\python"; Description: "Python"; Types: full custom; Flags: disablenouninstallwarning
Name: "langs\tcl"; Description: "Tcl"; Types: full custom; Flags: disablenouninstallwarning

[Tasks]
Name: x86; Description: "x86"; GroupDescription: "XChat-WDK version:"; Flags: exclusive unchecked
Name: x64; Description: "x64"; GroupDescription: "XChat-WDK version:"; Flags: exclusive; Check: Is64BitInstallMode

Name: perl512; Description: "5.12"; GroupDescription: "Perl version:"; Flags: exclusive; Components: langs\perl
Name: perl514; Description: "5.14"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl

Name: portable; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked

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
Source: "cert.pem"; DestDir: "{app}"; Components: libs
Source: "COPYING"; DestDir: "{app}"; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Components: libs
Source: "LICENSE.GTK"; DestDir: "{app}"; Components: libs
Source: "LICENSE.CAIRO"; DestDir: "{app}"; Components: libs
Source: "LICENSE.LUA"; DestDir: "{app}"; Components: libs
Source: "LICENSE.ENCHANT"; DestDir: "{app}"; Components: libs
Source: "portable-mode"; DestDir: "{app}"; Tasks: portable
Source: "etc\*"; DestDir: "{app}\etc"; Flags: createallsubdirs recursesubdirs; Components: libs
Source: "locale\*"; DestDir: "{app}\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
Source: "share\locale\*"; DestDir: "{app}\share\locale"; Flags: createallsubdirs recursesubdirs; Components: translations
;Source: "share\myspell\*"; DestDir: "{app}\share\myspell"; Flags: createallsubdirs recursesubdirs; Components: spelling



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

Source: "libenchant.dll"; DestDir: "{app}"; Components: libs; Tasks: x86
Source: "libenchant.dll.x64"; DestDir: "{app}"; DestName: "libenchant.dll"; Components: libs; Tasks: x64

Source: "lib\enchant\libenchant_myspell.dll"; DestDir: "{app}\lib\enchant"; Components: libs; Tasks: x86
Source: "lib\enchant\libenchant_myspell.dll.x64"; DestDir: "{app}\lib\enchant"; DestName: "libenchant_myspell.dll"; Components: libs; Tasks: x64



Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll.x64"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; DestName: "libpixmap.dll"; Components: libs; Tasks: x64

Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll.x64"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; DestName: "libwimp.dll"; Components: libs; Tasks: x64

Source: "lib\gtk-2.0\modules\libgail.dll"; DestDir: "{app}\lib\gtk-2.0\modules"; Components: libs; Tasks: x86
Source: "lib\gtk-2.0\modules\libgail.dll.x64"; DestDir: "{app}\lib\gtk-2.0\modules"; DestName: "libgail.dll"; Components: libs; Tasks: x64



Source: "xchat-text.exe"; DestDir: "{app}"; Components: xctext; Tasks: x86
Source: "xchat-text.exe.x64"; DestDir: "{app}"; DestName: "xchat-text.exe"; Components: xctext; Tasks: x64



;Source: "plugins\xcewc.dll"; DestDir: "{app}\plugins"; Components: plugins\ewc; Tasks: x86
;Source: "plugins\xcewc.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcewc.dll"; Components: plugins\ewc; Tasks: x64

Source: "plugins\xcchecksum.dll"; DestDir: "{app}\plugins"; Components: plugins\checksum; Tasks: x86
Source: "plugins\xcchecksum.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcchecksum.dll"; Components: plugins\checksum; Tasks: x64

Source: "plugins\xcmpcinfo.dll"; DestDir: "{app}\plugins"; Components: plugins\mpcinfo; Tasks: x86
Source: "plugins\xcmpcinfo.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcmpcinfo.dll"; Components: plugins\mpcinfo; Tasks: x64

Source: "plugins\xcupd.dll"; DestDir: "{app}\plugins"; Components: plugins\upd; Tasks: x86
Source: "plugins\xcupd.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcupd.dll"; Components: plugins\upd; Tasks: x64

Source: "plugins\xcwinamp.dll"; DestDir: "{app}\plugins"; Components: plugins\winamp; Tasks: x86
Source: "plugins\xcwinamp.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcwinamp.dll"; Components: plugins\winamp; Tasks: x64

;Source: "plugins\xcxdcc.dll"; DestDir: "{app}\plugins"; Components: plugins\xdcc; Tasks: x86
;Source: "plugins\xcxdcc.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcxdcc.dll"; Components: plugins\xdcc; Tasks: x64

Source: "plugins\xtray.dll"; DestDir: "{app}\plugins"; Components: plugins\xtray; Tasks: x86
Source: "plugins\xtray.dll.x64"; DestDir: "{app}\plugins"; DestName: "xtray.dll"; Components: plugins\xtray; Tasks: x64



Source: "plugins\xclua.dll"; DestDir: "{app}\plugins"; Components: langs\lua; Tasks: x86
Source: "plugins\xclua.dll.x64"; DestDir: "{app}\plugins"; DestName: "xclua.dll"; Components: langs\lua; Tasks: x64

Source: "plugins\xcpython.dll"; DestDir: "{app}\plugins"; Components: langs\python; Tasks: x86
Source: "plugins\xcpython.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcpython.dll"; Components: langs\python; Tasks: x64

Source: "plugins\xctcl.dll"; DestDir: "{app}\plugins"; Components: langs\tcl; Tasks: x86
Source: "plugins\xctcl.dll.x64"; DestDir: "{app}\plugins"; DestName: "xctcl.dll"; Components: langs\tcl; Tasks: x64



Source: "plugins\xcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: x86 and perl512
Source: "plugins\xcperl-512.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: x64 and perl512

Source: "plugins\xcperl-514.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: x86 and perl514
Source: "plugins\xcperl-514.dll.x64"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Components: langs\perl; Tasks: x64 and perl514

[Icons]
Name: "{group}\XChat-WDK"; Filename: "{app}\xchat.exe"; Tasks: not portable
Name: "{group}\XChat-Text"; Filename: "{app}\xchat-text.exe"; Components: xctext; Tasks: not portable
Name: "{group}\Uninstall XChat-WDK"; Filename: "{uninstallexe}"; Tasks: not portable

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
