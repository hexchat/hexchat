AppName=XChat-WDK (x86)
AppPublisher=XChat-WDK
AppPublisherURL=http://www.xchat-wdk.org/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://www.xchat-wdk.org/home/downloads
LicenseFile=COPYING
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK (x86)
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
SolidCompression=yes
SourceDir=..\rel
OutputDir=..
FlatComponentsList=no
PrivilegesRequired=none
ShowComponentSizes=no
CreateUninstallRegKey=not IsTaskSelected('portable')
Uninstallable=not IsTaskSelected('portable')
ArchitecturesAllowed=x86 x64

[Types]
Name: "normal"; Description: "Normal Installation"
Name: "full"; Description: "Full Installation"
Name: "minimal"; Description: "Minimal Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "libs"; Description: "XChat-WDK"; Types: normal full minimal custom; Flags: fixed
Name: "xctext"; Description: "XChat-Text"; Types: full custom; Flags: disablenouninstallwarning
Name: "xtm"; Description: "XChat Theme Manager (Requires .NET 4.0)"; Types: full custom; Flags: disablenouninstallwarning
Name: "translations"; Description: "Translations"; Types: normal full custom; Flags: disablenouninstallwarning
;obs Name: "gtkengines"; Description: "GTK+ Engines"; Types: full custom; Flags: disablenouninstallwarning
;Name: "spelling"; Description: "Spelling Dictionaries"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins"; Description: "Plugins"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\checksum"; Description: "Checksum"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\dns"; Description: "DNS"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\doat"; Description: "Do At"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\exec"; Description: "Exec"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\fishlim"; Description: "FiSHLiM"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\mpcinfo"; Description: "mpcInfo"; Types: full custom; Flags: disablenouninstallwarning
;Name: "plugins\nonbmp"; Description: "Non-BMP"; Types: normal full custom; Flags: disablenouninstallwarning
Name: "plugins\upd"; Description: "Update Checker"; Types: normal full custom; Flags: disablenouninstallwarning
Name: "plugins\winamp"; Description: "Winamp"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\winsys"; Description: "WinSys"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\wmpa"; Description: "Windows Media Player Announcer"; Types: full custom; Flags: disablenouninstallwarning
Name: "plugins\xsasl"; Description: "X-SASL"; Types: full custom; Flags: disablenouninstallwarning
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
Name: perl516; Description: "5.16"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl

[Registry]
Root: HKCR; Subkey: "irc"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey uninsdeletekey; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: ""; ValueData: "URL:IRC Protocol"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\xchat.exe,0"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell\open\command"; ValueType: string; ValueName: ""; ValueData: "{app}\xchat.exe --url=""%1"""; Flags: uninsdeletevalue; Tasks: not portable

Root: HKCR; Subkey: ".xct"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey uninsdeletekey; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".xct"; ValueType: string; ValueName: ""; ValueData: "XChat Theme File"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".xct"; ValueType: string; ValueName: "XChat Theme File"; ValueData: ""; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".xct\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\thememan.exe,0"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".xct\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".xct\shell\open\command"; ValueType: string; ValueName: ""; ValueData: "{app}\thememan.exe ""%1"""; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable

[Run]
Filename: "{app}\xchat.exe"; Description: "Run XChat-WDK after closing the Wizard"; Flags: nowait postinstall skipifsilent

[Files]
; Add the ISSkin DLL used for skinning Inno Setup installations.
Source: ISSkinU.dll; DestDir: {app}; Flags: dontcopy

; Add the Visual Style resource contains resources used for skinning,
; you can also use Microsoft Visual Styles (*.msstyles) resources.
Source: watercolorlite-green.cjstyles; DestDir: {tmp}; Flags: dontcopy

Source: "portable-mode"; DestDir: "{app}"; Tasks: portable

Source: "cert.pem"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "COPYING"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.GTK"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.CAIRO"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.LUA"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.ENCHANT"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.LIBXML"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; Components: libs
;Source: "etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; Components: libs and not gtkengines
Source: "share\xml\*"; DestDir: "{app}\share\xml"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: libs
Source: "locale\*"; DestDir: "{app}\locale"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: translations
Source: "share\locale\*"; DestDir: "{app}\share\locale"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: translations
;Source: "share\myspell\*"; DestDir: "{app}\share\myspell"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: spelling

Source: "libatk-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libcairo-2.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libexpat-1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libffi-5.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "freetype6.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libfreetype-6.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libfontconfig-1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgdk_pixbuf-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgdk-win32-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgio-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libglib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgmodule-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgobject-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgthread-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libgtk-win32-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "intl.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libintl-8.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libjasper-1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libjpeg-8.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpango-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpangocairo-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpangoft2-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpangowin32-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libpixman-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libtiff-3.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpng14-14.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libpng15-15.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "lua51.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "zlib1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libxml2.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;obs Source: "libxml2-2.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libenchant.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs

Source: "lib\enchant\libenchant_myspell.dll"; DestDir: "{app}\lib\enchant"; Flags: ignoreversion; Components: libs

Source: "lib\gtk-2.0\2.10.0\engines\libpixmap.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: libs
Source: "lib\gtk-2.0\2.10.0\engines\libwimp.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: libs
Source: "lib\gtk-2.0\modules\libgail.dll"; DestDir: "{app}\lib\gtk-2.0\modules"; Flags: ignoreversion; Components: libs

;obs Source: "etc\gtkpref.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libclearlooks.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libcrux-engine.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libglide.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libhcengine.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libindustrial.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libmist.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libmurrine.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libredmond95.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "lib\gtk-2.0\2.10.0\engines\libthinice.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines"; Flags: ignoreversion; Components: gtkengines
;obs Source: "plugins\xcgtkpref.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: gtkengines
;obs Source: "share\themes\*"; DestDir: "{app}\share\themes"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: gtkengines
;obs Source: "gtk2-prefs.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: gtkengines

Source: "plugins\xcchecksum.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\checksum
Source: "plugins\xcdns.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\dns
Source: "plugins\xcdoat.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\doat
Source: "plugins\xcexec.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\exec
Source: "plugins\xcfishlim.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\fishlim
Source: "etc\music.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\winamp or plugins\mpcinfo
Source: "plugins\xcmpcinfo.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\mpcinfo
;Source: "plugins\xcnonbmp.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\nonbmp
Source: "etc\download.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\upd
Source: "plugins\xcupd.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\upd
Source: "plugins\xcwinamp.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\winamp
Source: "etc\system.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\winsys
Source: "plugins\xcwinsys.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\winsys
Source: "plugins\xcxsasl.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\xsasl
Source: "plugins\xtray.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\xtray
Source: "plugins\xcwmpa.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\wmpa

Source: "plugins\xclua.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\lua
Source: "plugins\xcpython.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\python
Source: "plugins\xctcl.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\tcl

Source: "plugins\xcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl512
Source: "plugins\xcperl-514.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl514
Source: "plugins\xcperl-516.dll"; DestDir: "{app}\plugins"; DestName: "xcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl516

Source: "xchat.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "xchat-text.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xctext
Source: "thememan.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xtm

[Icons]
Name: "{group}\XChat-WDK (x86)"; Filename: "{app}\xchat.exe"; Tasks: not portable
Name: "{group}\XChat-Text (x86)"; Filename: "{app}\xchat-text.exe"; Components: xctext; Tasks: not portable
Name: "{group}\XChat Theme Manager (x86)"; Filename: "{app}\thememan.exe"; Components: xtm; Tasks: not portable
Name: "{group}\Uninstall XChat-WDK (x86)"; Filename: "{uninstallexe}"; Tasks: not portable

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
	sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\XChat-WDK (x86)_is1');
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

/////////////////////////////////////////////////////////////////////
// Importing LoadSkin API from ISSkin.DLL
procedure LoadSkin(lpszPath: String; lpszIniFileName: String);
external 'LoadSkin@files:isskinu.dll stdcall';

// Importing UnloadSkin API from ISSkin.DLL
procedure UnloadSkin();
external 'UnloadSkin@files:isskinu.dll stdcall';

// Importing ShowWindow Windows API from User32.DLL
function ShowWindow(hWnd: Integer; uType: Integer): Integer;
external 'ShowWindow@user32.dll stdcall';

function InitializeSetup(): Boolean;
begin
  ExtractTemporaryFile('watercolorlite-green.cjstyles');
  LoadSkin(ExpandConstant('{tmp}\watercolorlite-green.cjstyles'), '');
  Result := True;
end;

procedure DeinitializeSetup();
begin
  // Hide Window before unloading skin so user does not get
  // a glimpse of an unskinned window before it is closed.
  ShowWindow(StrToInt(ExpandConstant('{wizardhwnd}')), 0);
  UnloadSkin();
end;
