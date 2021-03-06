; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "NZBDrive"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "ByteFountain"
#define MyAppURL "http://www.nzbdrive.com/"
#define MyAppExeName "NZBDrive.exe"
#define MyRootDir "..\..\"
#define MyAppGuid "C8F05E06-E5FF-4503-9151-F45CA45B6E51"
#define MyVSVer "14.0"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{{#MyAppGuid}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
VersionInfoVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile={#MyRootDir}\Windows\NZBDriveInnoSetup\License.rtf
OutputDir={#MyRootDir}
OutputBaseFilename=nzbdrive-setup
SetupIconFile={#MyRootDir}\Windows\NZBDriveNone.ico
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
MinVersion=6.0
AlwaysRestart=no
; RestartIfNeededByRun=no - since the .net 4.5 installer will cause an unneccesary reboot 
RestartIfNeededByRun=no
UninstallRestartComputer=yes


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
OpenNZBWithNZBFile=Open NZB files with NZBDrive

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; 
Name: "opennzbfiles"; Description: "{cm:OpenNZBWithNZBFile}"; 

[Files]
; DOCS
Source: "license.mit.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "license.lgpl.txt"; DestDir: "{app}"; Flags: ignoreversion
; NZBDrive
Source: "{#MyRootDir}\x86\Release\NZBDrive.exe"; DestDir: "{app}"; Check: not Is64BitInstallMode 
Source: "{#MyRootDir}\x64\Release\NZBDrive.exe"; DestDir: "{app}"; Check: Is64BitInstallMode 
Source: "{#MyRootDir}\x86\Release\NZBDriveDLL.dll"; DestDir: "{app}"; Check: not Is64BitInstallMode
Source: "{#MyRootDir}\x64\Release\NZBDriveDLL.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
Source: "{#MyRootDir}\x86\Release\dokan1.dll"; DestDir: "{app}"; Check: not Is64BitInstallMode
Source: "{#MyRootDir}\x64\Release\dokan1.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
Source: "{#MyRootDir}\x86\Release\dokanfuse1.dll"; DestDir: "{app}"; Check: not Is64BitInstallMode
Source: "{#MyRootDir}\x64\Release\dokanfuse1.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
Source: "{#MyRootDir}\x86\Release\libcrypto-3.dll"; DestDir: "{app}"; Check: not Is64BitInstallMode
Source: "{#MyRootDir}\x64\Release\libcrypto-3-x64.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
Source: "{#MyRootDir}\x86\Release\libssl-3.dll"; DestDir: "{app}"; Check: not Is64BitInstallMode
Source: "{#MyRootDir}\x64\Release\libssl-3-x64.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
; Xceed.Wpf.Toolkit.dll
Source: "{#MyRootDir}\x86\Release\Xceed.Wpf.Toolkit.dll"; DestDir: "{app}"; 
; .NET 4.5
Source: "dotNetFx45_Full_setup.exe"; DestDir: "{app}"; 
; Dokan
Source: "DokanSetup.exe"; DestDir: "{app}"; 
; Microsoft.VisualStudio.Shell.Immutable.10.0.dll
Source: "{#MyRootDir}\x86\Release\Microsoft.VisualStudio.Shell.Immutable.10.0.dll"; DestDir: "{app}";
; MS Redist
Source: "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Redist\MSVC\14.16.27012\vcredist_x64.exe"; DestDir: "{app}"; Check: Is64BitInstallMode
Source: "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Redist\MSVC\14.16.27012\vcredist_x86.exe"; DestDir: "{app}"; Check: not Is64BitInstallMode


; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Registry]
Root: HKCR; Subkey: ".nzb"; ValueType: string; ValueName: ""; ValueData: "NZBDrive"; Flags: uninsdeletevalue; Tasks: opennzbfiles
Root: HKCR; Subkey: "NZBDrive"; ValueType: string; ValueName: ""; ValueData: "NZBDrive File"; Flags: uninsdeletekey; Tasks: opennzbfiles
Root: HKCR; Subkey: "NZBDrive\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\NZBDrive.EXE,0"; Tasks: opennzbfiles
Root: HKCR; Subkey: "NZBDrive\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\NZBDrive.EXE"" ""%1"""; Tasks: opennzbfiles
Root: HKCR; Subkey: "Applications\NZBDrive"; ValueType: string; ValueName: ""; ValueData: "NZBDrive File"; Flags: uninsdeletekey; Tasks: opennzbfiles; Check: Is64BitInstallMode
Root: HKCR; Subkey: "Applications\NZBDrive\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\NZBDrive.EXE,0"; Tasks: opennzbfiles; Check: Is64BitInstallMode
Root: HKCR; Subkey: "Applications\NZBDrive\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\NZBDrive.EXE"" ""%1"""; Tasks: opennzbfiles; Check: Is64BitInstallMode

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\dotNetFx45_Full_setup.exe"; Parameters: "/q:a /c:""install /l /q"""; StatusMsg: "Microsoft Framework 4.5 is being installed. Please wait... "; Check: Framework45IsNotInstalled()
Filename: "{app}\DokanSetup.exe"; Parameters: "/install /quiet"; StatusMsg: "Dokan Library is being installed. Please wait... "; 
Filename: "{app}\vcredist_x86.exe"; StatusMsg: "Visual Studio C++ Redistributable is being installed. Please wait... "; Check: not Is64BitInstallMode()
Filename: "{app}\vcredist_x64.exe"; StatusMsg: "Visual Studio C++ Redistributable is being installed. Please wait... "; Check: Is64BitInstallMode()


[Code]
function Framework45IsNotInstalled: Boolean;
var
 bVer4x5: Boolean;
 bSuccess: Boolean;
 iInstalled: Cardinal;
 strVersion: String;
 iPos: Cardinal;
begin
 Result := True;
 bVer4x5 := False;

 bSuccess := RegQueryDWordValue(HKLM, 'Software\Microsoft\NET Framework Setup\NDP\v4\Full', 'Install', iInstalled);
 if (1 = iInstalled) AND (True = bSuccess) then
  begin
    bSuccess := RegQueryStringValue(HKLM, 'Software\Microsoft\NET Framework Setup\NDP\v4\Full', 'Version', strVersion);
    if (True = bSuccess) then
     Begin
        iPos := Pos('4.5.', strVersion);
        if (0 < iPos) then bVer4x5 := True;
     End
  end;

 if (True = bVer4x5) then begin
    Result := False;
 end;
end;


function GetUninstallString: string;
var
  sUnInstPath: string;
  sUnInstallString: String;
begin
  Result := '';
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{{{#MyAppGuid}}_is1'); //Your App GUID/ID
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;

function IsUpgrade: Boolean;
begin
  Result := (GetUninstallString <> '');
end;

function CheckPreviousInstallation: Boolean;
var
  V: Integer;
  iResultCode: Integer;
  sUnInstallString: string;
begin
  Result := True; // in case when no previous version is found
  if RegValueExists(HKEY_LOCAL_MACHINE,'Software\Microsoft\Windows\CurrentVersion\Uninstall\{{#MyAppGuid}}_is1', 'UninstallString') then
  begin
//Your App GUID/ID
    V := MsgBox(ExpandConstant('An old version of NZBDrive was detected. We recommend that you uninstall first. Do you want to uninstall it?'), mbInformation, MB_YESNO); //Custom Message if App installed
    if V = IDYES then
    begin
      sUnInstallString := GetUninstallString();
      sUnInstallString :=  RemoveQuotes(sUnInstallString);
      Exec(ExpandConstant(sUnInstallString), '', '', SW_SHOW, ewWaitUntilTerminated, iResultCode);
//      Result := True; //if you want to proceed after uninstall
      Result := false; //if you want to proceed after uninstall
                //Exit; //if you want to quit after uninstall
    end
    else
      Result := False; //when older version present and not uninstalled
  end;
end;

function InitializeSetup(): Boolean;
begin
    result := CheckPreviousInstallation();
end;
