[Setup]
OutputBaseFileName=nsp-current
VersionInfoVersion=0.9.4
AppVerName=NullLogic NSP Script Parser 0.9.4

AppName=NullLogic NSP Script Parser
AppID={{8C6107AB-FCB7-42A5-BB18-F136497FDBEE}
AppCopyright=Copyright 2022 Dan Cahill
AppPublisher=NullLogic
AppPublisherURL=https://nulllogic.ca/nsp/
AppSupportURL=https://nulllogic.ca/nsp/
AppUpdatesURL=https://nulllogic.ca/nsp/
DefaultDirName={commonpf64}\NSP
SetupIconFile=".\src\hosts\ntray\ntray1.ico"
DefaultGroupName=NullLogic NSP Script Parser
LicenseFile=.\doc\copyright.txt
;InfoAfterFile=.\doc\README
OutputDir=.\
ChangesAssociations=yes

[Registry]
Root: HKCR; Subkey: ".ns"; ValueType: string; ValueName: ""; ValueData: "NSP Script"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "NSP Script"; ValueType: string; ValueName: ""; ValueData: "NSP Script"; Flags: uninsdeletekey
Root: HKCR; Subkey: "NSP Script\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{win}\NSP.EXE,0"
;Root: HKCR; Subkey: "NSP Script\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{win}\NSP.EXE"" ""%1"""
Root: HKCR; Subkey: "NSP Script\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{win}\NSP.EXE"" ""%1"" %*"
;Root: HKCR; Subkey: "NSP Script\shell\edit\command"; ValueType: string; ValueName: ""; ValueData: """C:\Program Files\Programmer's Notepad\pn.exe"" ""%1"""
Root: HKCR; Subkey: "NSP Script\shell\edit\command"; ValueType: string; ValueName: ""; ValueData: """{app}\nspedit.exe"" ""%1"""

[Files]
Source: ".\bin\nsp.exe";                    DestDir: "{app}"
;Source: ".\bin\nsp-cgi.exe";                DestDir: "{app}"
Source: ".\bin\nsp.exe";                    DestDir: "{win}"
Source: ".\bin\nspedit.exe";                DestDir: "{app}"
Source: ".\bin\nsp.net.dll";                DestDir: "{app}"
Source: ".\bin\ntray.exe";                  DestDir: "{app}"
Source: ".\src\hosts\ntray\ntray.conf";     DestDir: "{app}"; DestName: "ntray.conf-sample"
Source: ".\include\nsp\nsp*.h";             DestDir: "{app}\include\nsp"
Source: ".\lib\libnsp.lib";                 DestDir: "{app}\lib"
Source: ".\lib\shared\*.dll";               DestDir: "{app}\lib"
;OpenSSL
Source: "C:\NullLogic\utils\VS2019\lib\lib*.dll"; DestDir: "{app}"
Source: ".\src\libs\preload.ns";            DestDir: "{app}\lib"; DestName: "preload-sample.ns"
;Source: ".\scripts\samples\*.*";            DestDir: "{app}\scripts\samples"; Flags: recursesubdirs;
Source: ".\tests\*.*";                      DestDir: "{app}\tests"; Flags: recursesubdirs;
Source: ".\doc\copying.html";               DestDir: "{app}\doc"
Source: ".\doc\copyright.html";             DestDir: "{app}\doc"
Source: ".\doc\syntax.html";                DestDir: "{app}\doc"

[Icons]
Name: "{group}\NTray";            Filename: "{app}\NTray.exe";              Workingdir: "{app}"
Name: "{group}\NSP Editor";       Filename: "{app}\NSPEdit.exe";            Workingdir: "{app}"
Name: "{group}\NSP Copyright";    Filename: "{app}\doc\copyright.html";     Workingdir: "{app}\doc"
Name: "{group}\NSP Scripts";      Filename: "{app}\scripts";                Workingdir: "{app}\scripts"
Name: "{group}\NSP Syntax";       Filename: "{app}\doc\syntax.html";        Workingdir: "{app}\doc"
Name: "{group}\NullLogic Online"; Filename: "https://nulllogic.ca/nsp/"
;Name: "{commonstartup}\NTray"; Workingdir: "{app}"; Filename: "{app}\ntray.exe"

[Run]
;Filename: "{app}\ntray.exe"; Workingdir: "{app}"; Description: "Launch NTray"; Flags: nowait postinstall skipifsilent

[UninstallRun]
;Filename: "net.exe"; Parameters: "stop nullgroupware"; Flags: runminimized

[Code]
const
  RegKeyBase     = 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{8C6107AB-FCB7-42A5-BB18-F136497FDBEE}_is1';

function InitializeSetup(): Boolean;
var
  msgRes : integer;
  msgline : string;
  uninstaller : string;
  uninstalloc : string;
  ResultCode : integer;

begin
  Result := true;
  if (RegKeyExists(HKLM, RegKeyBase)) then begin
    msgline:='NSP appears to be installed already.';
    msgline:=msgline+#13+#10+#13+#10+'You must uninstall the previous instance before installing this module.';
    msgline:=msgline+#13+#10+#13+#10+'Uninstall old version?';
    if MsgBox(msgline, mbConfirmation, MB_YESNO) = IDYES then begin
      RegQueryStringValue(HKLM, RegKeyBase, 'QuietUninstallString', uninstaller);
      RegQueryStringValue(HKLM, RegKeyBase, 'InstallLocation', uninstalloc);
      if Exec(RemoveQuotes(uninstaller), '', uninstalloc, SW_SHOW, ewWaitUntilTerminated, ResultCode) then begin
        if (RegKeyExists(HKLM, RegKeyBase)) then begin
          msgline:='NSP appears to be installed already.';
          msgline:=msgline+#13+#10+#13+#10+'You must uninstall the previous instance before installing this module.';
          msgline:=msgline+#13+#10+#13+#10+'This installer will exit now.';
          msgRes := MsgBox(msgline, mbError, MB_OK);
          Result := False;
        end else begin
          Result := True;
        end;
      end else begin
        Result := False;
      end;
    end else begin
      msgline:='NSP appears to be installed already.';
      msgline:=msgline+#13+#10+#13+#10+'You must uninstall the previous instance before installing this module.';
      msgline:=msgline+#13+#10+#13+#10+'This installer will exit now.';
      msgRes := MsgBox(msgline, mbError, MB_OK);
      Result := False;
    end;
  end;
end;
[/Code]

