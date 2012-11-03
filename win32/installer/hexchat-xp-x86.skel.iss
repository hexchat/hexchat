AppName=HexChat (x86)
AppPublisher=HexChat
AppPublisherURL=http://www.hexchat.org/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=https://github.com/hexchat/hexchat/issues
AppUpdatesURL=http://www.hexchat.org/home/downloads
LicenseFile=COPYING
UninstallDisplayIcon={app}\hexchat.exe
UninstallDisplayName=HexChat (x86)
DefaultDirName={pf}\HexChat
DefaultGroupName=HexChat
DisableProgramGroupPage=yes
SolidCompression=yes
Compression=lzma2/ultra64
SourceDir=..\rel
OutputDir=..
FlatComponentsList=no
PrivilegesRequired=none
ShowComponentSizes=no
CreateUninstallRegKey=not IsTaskSelected('portable')
Uninstallable=not IsTaskSelected('portable')
ArchitecturesAllowed=x86 x64
MinVersion=5.1

[Types]
Name: "normal"; Description: "Normal Installation"
Name: "minimal"; Description: "Minimal Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "libs"; Description: "HexChat"; Types: normal minimal custom; Flags: fixed
Name: "gtktheme"; Description: "GTK+ Theme"; Types: normal custom; Flags: disablenouninstallwarning
Name: "xctext"; Description: "HexChat-Text"; Types: custom; Flags: disablenouninstallwarning
Name: "xtm"; Description: "HexChat Theme Manager (Requires .NET 4.0)"; Types: custom; Flags: disablenouninstallwarning
Name: "translations"; Description: "Translations"; Types: normal custom; Flags: disablenouninstallwarning
;obs Name: "gtkengines"; Description: "GTK+ Engines"; Types: custom; Flags: disablenouninstallwarning
;Name: "spelling"; Description: "Spelling Dictionaries"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins"; Description: "Plugins"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\checksum"; Description: "Checksum"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\dns"; Description: "DNS"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\doat"; Description: "Do At"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\exec"; Description: "Exec"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\fishlim"; Description: "FiSHLiM"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\hextray"; Description: "HexTray"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\mpcinfo"; Description: "mpcInfo"; Types: custom; Flags: disablenouninstallwarning
;Name: "plugins\sasl"; Description: "SASL"; Types: normal custom; Flags: disablenouninstallwarning
Name: "plugins\sysinfo"; Description: "SysInfo"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\upd"; Description: "Update Checker"; Types: normal custom; Flags: disablenouninstallwarning
Name: "plugins\winamp"; Description: "Winamp"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\wmpa"; Description: "Windows Media Player Announcer"; Types: custom; Flags: disablenouninstallwarning
Name: "langs"; Description: "Language Interfaces"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\lua"; Description: "Lua"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\lua\luawdk"; Description: "Lua-WDK"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\perl"; Description: "Perl"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\python"; Description: "Python"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\tcl"; Description: "Tcl"; Types: custom; Flags: disablenouninstallwarning

[Tasks]
Name: portable; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked

Name: perl512; Description: "5.12"; GroupDescription: "Perl version:"; Flags: exclusive; Components: langs\perl
Name: perl514; Description: "5.14"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl
Name: perl516; Description: "5.16"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl

[Registry]
Root: HKCR; Subkey: "irc"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey uninsdeletekey; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: ""; ValueData: "URL:IRC Protocol"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\hexchat.exe,0"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletevalue; Tasks: not portable
Root: HKCR; Subkey: "irc\shell\open\command"; ValueType: string; ValueName: ""; ValueData: "{app}\hexchat.exe --url=""%1"""; Flags: uninsdeletevalue; Tasks: not portable

Root: HKCR; Subkey: ".hct"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey uninsdeletekey; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".hct"; ValueType: string; ValueName: ""; ValueData: "HexChat Theme File"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".hct"; ValueType: string; ValueName: "HexChat Theme File"; ValueData: ""; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".hct\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\thememan.exe,0"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".hct\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable
Root: HKCR; Subkey: ".hct\shell\open\command"; ValueType: string; ValueName: ""; ValueData: "{app}\thememan.exe ""%1"""; Flags: uninsdeletevalue; Components:xtm; Tasks: not portable

[Run]
Filename: "{app}\hexchat.exe"; Description: "Run HexChat after closing the Wizard"; Flags: nowait postinstall skipifsilent

[Files]
; Add the ISSkin DLL used for skinning Inno Setup installations.
Source: ISSkinU.dll; DestDir: {app}; Flags: dontcopy

; Add the Visual Style resource contains resources used for skinning,
; you can also use Microsoft Visual Styles (*.msstyles) resources.
Source: watercolorlite-green.cjstyles; DestDir: {tmp}; Flags: dontcopy

Source: "portable-mode"; DestDir: "{app}"; Tasks: portable

Source: "changelog.url"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "readme.url"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "cert.pem"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "COPYING"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.OPENSSL"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.ZLIB"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.GTK"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.CAIRO"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.LUA"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.ENCHANT"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "LICENSE.LIBXML"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; Components: gtktheme
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
;obs Source: "plugins\hcgtkpref.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: gtkengines
;obs Source: "share\themes\*"; DestDir: "{app}\share\themes"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: gtkengines
;obs Source: "gtk2-prefs.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: gtkengines

Source: "plugins\hcchecksum.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\checksum
Source: "plugins\hcdns.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\dns
Source: "plugins\hcdoat.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\doat
Source: "plugins\hcexec.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\exec
Source: "plugins\hcfishlim.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\fishlim
Source: "etc\music.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\winamp or plugins\mpcinfo
Source: "plugins\hcmpcinfo.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\mpcinfo
Source: "etc\download.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\upd
Source: "plugins\hcupd.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\upd
Source: "plugins\hcwinamp.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\winamp
Source: "etc\system.png"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: plugins\sysinfo
Source: "plugins\hcsysinfo.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\sysinfo
;Source: "plugins\hcsasl.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\sasl
Source: "plugins\hchextray.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\hextray
Source: "plugins\hcwmpa.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\wmpa

Source: "plugins\hclua.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\lua
Source: "plugins\hcpython.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\python
Source: "plugins\hctcl.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\tcl

Source: "plugins\hcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl512
Source: "plugins\hcperl-514.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl514
Source: "plugins\hcperl-516.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl516

Source: "hexchat.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "hexchat-text.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xctext
Source: "thememan.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xtm

[Icons]
Name: "{group}\HexChat (x86)"; Filename: "{app}\hexchat.exe"; Tasks: not portable
Name: "{group}\HexChat (x86) ChangeLog"; Filename: "{app}\changelog.url"; IconFilename: "{sys}\shell32.dll"; IconIndex: 165; Tasks: not portable
Name: "{group}\HexChat (x86) ReadMe"; Filename: "{app}\readme.url"; IconFilename: "{sys}\shell32.dll"; IconIndex: 23; Tasks: not portable
Name: "{group}\HexChat-Text (x86)"; Filename: "{app}\hexchat-text.exe"; Components: xctext; Tasks: not portable
Name: "{group}\HexChat Theme Manager (x86)"; Filename: "{app}\thememan.exe"; Components: xtm; Tasks: not portable
Name: "{group}\Uninstall HexChat (x86)"; Filename: "{uninstallexe}"; Tasks: not portable

[Messages]
BeveledLabel= HexChat

[Code]
/////////////////////////////////////////////////////////////////////
procedure InitializeWizard;
begin
	WizardForm.LicenseAcceptedRadio.Checked := True;
end;


/////////////////////////////////////////////////////////////////////
// these are required for x86->x64 or reverse upgrades
/////////////////////////////////////////////////////////////////////
function GetUninstallString(): String;
var
	sUnInstPath: String;
	sUnInstallString: String;
begin
	sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\HexChat (x86)_is1');
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
procedure MigrateConf();
begin
  FileCopy(ExpandConstant('{userappdata}\HexChat\xchat.conf'), ExpandConstant('{userappdata}\HexChat\hexchat.conf'), True);
end;

/////////////////////////////////////////////////////////////////////
function ConfExistCheck(): Boolean;
begin
  if FileExists(ExpandConstant('{userappdata}\HexChat\xchat.conf')) then
      Result := True
  else
    Result := False
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
			DeleteFile(ExpandConstant('{app}\portable-mode'));
		end;

		if (CurStep=ssPostInstall) then
		begin
			if ConfExistCheck() then begin
				if SuppressibleMsgBox('Would you like to copy your old HexChat configuration file (xchat.conf) to the new name (hexchat.conf)? Make sure you remove xchat.conf when you no longer need it.', mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
					MigrateConf();
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
