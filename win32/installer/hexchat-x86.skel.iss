AppName=HexChat (x86)
AppPublisher=HexChat
AppPublisherURL=http://www.hexchat.org/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=https://github.com/hexchat/hexchat/issues
AppUpdatesURL=http://www.hexchat.org/home/downloads
LicenseFile=share\doc\hexchat\COPYING
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
Name: "plugins"; Description: "Plugins"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\checksum"; Description: "Checksum"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\dns"; Description: "DNS"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\doat"; Description: "Do At"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\exec"; Description: "Exec"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\fishlim"; Description: "FiSHLiM"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\mpcinfo"; Description: "mpcInfo"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\sysinfo"; Description: "SysInfo"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\upd"; Description: "Update Checker"; Types: normal custom; Flags: disablenouninstallwarning
Name: "plugins\winamp"; Description: "Winamp"; Types: custom; Flags: disablenouninstallwarning
Name: "plugins\wmpa"; Description: "Windows Media Player Announcer"; Types: custom; Flags: disablenouninstallwarning
Name: "langs"; Description: "Language Interfaces"; Types: custom; Flags: disablenouninstallwarning
;Name: "langs\lua"; Description: "Lua"; Types: custom; Flags: disablenouninstallwarning
;Name: "langs\lua\luawdk"; Description: "Lua-WDK"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\perl"; Description: "Perl"; Types: custom; Flags: disablenouninstallwarning
Name: "langs\python"; Description: "Python"; Types: custom; Flags: disablenouninstallwarning
;Name: "langs\tcl"; Description: "Tcl"; Types: custom; Flags: disablenouninstallwarning

[Tasks]
Name: portable; Description: "Yes"; GroupDescription: "Portable Install (no Registry entries, no Start Menu icons, no uninstaller):"; Flags: unchecked

;Name: perl512; Description: "5.12"; GroupDescription: "Perl version:"; Flags: exclusive; Components: langs\perl
;Name: perl514; Description: "5.14"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl
;Name: perl516; Description: "5.16"; GroupDescription: "Perl version:"; Flags: exclusive unchecked; Components: langs\perl

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
Filename: "https://hexchat.readthedocs.org/en/latest/changelog.html"; Description: "See what's changed"; Flags: shellexec runasoriginaluser postinstall skipifsilent unchecked
Filename: "http://www.microsoft.com/en-us/download/details.aspx?id=8328"; Description: "Download Visual C++ Redistributable Package"; Flags: shellexec runasoriginaluser postinstall skipifsilent unchecked

[Files]
Source: "portable-mode"; DestDir: "{app}"; Tasks: portable

Source: "changelog.url"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "readme.url"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "cert.pem"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; Components: gtktheme
;Source: "etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; Components: libs and not gtkengines
Source: "share\xml\*"; DestDir: "{app}\share\xml"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: libs
Source: "share\doc\*"; DestDir: "{app}\share\doc"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: libs
Source: "share\locale\*"; DestDir: "{app}\share\locale"; Flags: ignoreversion createallsubdirs recursesubdirs; Components: translations

Source: "atk-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "cairo.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "fontconfig.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gdk_pixbuf-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gdk-win32-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gio-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "glib-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gmodule-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gobject-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gthread-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "gtk-win32-2.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "harfbuzz.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "iconv.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libenchant.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libintl.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libpng15.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "libxml2.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
;Source: "lua51.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "pango-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "pangocairo-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "pangoft2-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "pangowin32-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "pixman-1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "zlib1.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: libs

Source: "lib\enchant\libenchant_myspell.dll"; DestDir: "{app}\lib\enchant"; Flags: ignoreversion; Components: libs

Source: "lib\gtk-2.0\i686-pc-vs10\engines\libwimp.dll"; DestDir: "{app}\lib\gtk-2.0\i686-pc-vs10\engines"; Flags: ignoreversion; Components: libs

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
Source: "plugins\hcwmpa.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: plugins\wmpa

;Source: "plugins\hclua.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\lua
Source: "plugins\hcpython.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\python
;Source: "plugins\hctcl.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Components: langs\tcl

;Source: "plugins\hcperl-512.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl512
;Source: "plugins\hcperl-514.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl; Tasks: perl514
Source: "plugins\hcperl-516.dll"; DestDir: "{app}\plugins"; DestName: "hcperl.dll"; Flags: ignoreversion; Components: langs\perl
; Tasks: perl516

Source: "hexchat.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: libs
Source: "hexchat-text.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xctext
Source: "thememan.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: xtm

[Icons]
Name: "{group}\HexChat (x86)"; Filename: "{app}\hexchat.exe"; Tasks: not portable
Name: "{group}\HexChat (x86) Safe Mode"; Filename: "{app}\hexchat.exe"; Parameters: "--no-auto --no-plugins"; Tasks: not portable
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
	end;
end;
