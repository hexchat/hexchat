[Setup] 
WizardImageFile=c:\mozilla build\build\xchat-wdk\win32\bitmaps\wizardimage.bmp 
WizardSmallImageFile=c:\mozilla build\build\xchat-wdk\win32\bitmaps\wizardsmallimage.bmp 
AppVerName=XChat-WDK 2.9.0
AppVersion=2.9.0
VersionInfoVersion=2.9.0
OutputBaseFilename=XChat-WDK 2.9.0 x64
SetupIconFile=c:\mozilla build\build\xchat-wdk\xchat.ico 
AppName=XChat-WDK
AppPublisher=XChat-WDK
AppPublisherURL=http://www.xchat-wdk.org/
AppCopyright=Copyright (C) 1998-2010 Peter Zelezny
AppSupportURL=http://code.google.com/p/xchat-wdk/issues/list
AppUpdatesURL=http://www.xchat-wdk.org/home/downloads
UninstallDisplayIcon={app}\xchat.exe
UninstallDisplayName=XChat-WDK
DefaultDirName={pf}\XChat-WDK
DefaultGroupName=XChat-WDK
DisableProgramGroupPage=yes
DisableDirPage=yes
DisableReadyPage=yes
DisableReadyMemo=yes
SolidCompression=yes
SourceDir=..\tmp
OutputDir=..\win32
FlatComponentsList=no
PrivilegesRequired=none
ShowComponentSizes=no
CreateUninstallRegKey=no
Uninstallable=no
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

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
	sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\XChat-WDK (x64)_is1');
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
procedure MigrateSettings();
begin
  RenameFile(ExpandConstant('{userappdata}\X-Chat 2'), ExpandConstant('{userappdata}\HexChat'));
end;

/////////////////////////////////////////////////////////////////////
function SettingsExistCheck(): Boolean;
begin
  if DirExists(ExpandConstant('{userappdata}\X-Chat 2')) then
    Result := True
  else
    Result := False
end;

/////////////////////////////////////////////////////////////////////
procedure CurStepChanged(CurStep: TSetupStep);
var
	ErrCode: integer;
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

		if (CurStep=ssPostInstall) then
		begin
			if SettingsExistCheck() then begin
				if SuppressibleMsgBox('Would you like to migrate your existing XChat configuration to HexChat?', mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
					MigrateSettings();
			end;
		end;

		if (CurStep=ssDone) then
		begin
			ShellExec('open', 'http://www.hexchat.org/home/downloads', '', '', SW_SHOW, ewNoWait, ErrCode);
		end;

	end;
end;
