; AI MIDI Composer - Inno Setup 5 Script
; Pure ASCII only

#define AppName     "AI MIDI Composer"
#define AppVersion  "0.1.0"
#define AppPublisher "Arik"
#define VST3Name    "AI MIDI Composer.vst3"
#define InstDir     "{pf}\AIMidiComposer"
#define SidecarDir  "{pf}\AIMidiComposer\sidecar"

[Setup]
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppId={{8a7f4b4c-3e5d-4e6a-9b12-21c9a7f58ab0}
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
DefaultDirName={pf}\AIMidiComposer
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableWelcomePage=no
AllowNoIcons=yes
OutputDir=dist
OutputBaseFilename=AIMidiComposer-Installer
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
UninstallDisplayName={#AppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[InstallDelete]
Type: filesandordirs; Name: "{pf}\Common Files\VST3\{#VST3Name}"
Type: filesandordirs; Name: "{pf}\AIMidiComposer\sidecar"
Type: filesandordirs; Name: "{pf}\AIMidiComposer\venv"

[Files]
Source: "build\AIMidiComposerVST_artefacts\Release\VST3\{#VST3Name}\*"; \
    DestDir: "{pf}\Common Files\VST3\{#VST3Name}"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

Source: "sidecar\dist\sidecar\*"; \
    DestDir: "{#SidecarDir}"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

Source: "sidecar\dist\venv_a.zip"; DestDir: "{#InstDir}"; Flags: ignoreversion
Source: "sidecar\dist\venv_b.zip"; DestDir: "{#InstDir}"; Flags: ignoreversion; Check: FileExists(ExpandConstant('{src}\sidecar\dist\venv_b.zip'))
Source: "sidecar\dist\venv_c.zip"; DestDir: "{#InstDir}"; Flags: ignoreversion; Check: FileExists(ExpandConstant('{src}\sidecar\dist\venv_c.zip'))
Source: "sidecar\dist\venv_d.zip"; DestDir: "{#InstDir}"; Flags: ignoreversion; Check: FileExists(ExpandConstant('{src}\sidecar\dist\venv_d.zip'))

Source: "docs\README.txt"; DestDir: "{#InstDir}"; Flags: ignoreversion

[Run]
Filename: "powershell.exe"; \
    Parameters: "-NoProfile -Command ""Get-ChildItem '{#InstDir}\venv_*.zip' | ForEach-Object {{ Expand-Archive -Path $_.FullName -DestinationPath '{#InstDir}\venv' -Force; Remove-Item $_.FullName -Force }}"""; \
    StatusMsg: "Extracting Python environment..."; \
    Flags: runhidden waituntilterminated

Filename: "netsh"; \
    Parameters: "advfirewall firewall add rule name=""AI MIDI Composer Sidecar"" dir=in action=allow program=""{pf}\AIMidiComposer\venv\Scripts\python.exe"" enable=yes profile=any"; \
    Flags: runhidden

[UninstallRun]
Filename: "netsh"; \
    Parameters: "advfirewall firewall delete rule name=""AI MIDI Composer Sidecar"""; \
    Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{pf}\Common Files\VST3\{#VST3Name}"
Type: filesandordirs; Name: "{pf}\AIMidiComposer"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var Msg: String;
begin
  if CurStep = ssDone then
  begin
    Msg := '{#AppName} installed.' + #13#10 + #13#10 +
           'VST3: C:\Program Files\Common Files\VST3\{#VST3Name}' + #13#10 +
           'Engine: C:\Program Files\AIMidiComposer\sidecar\' + #13#10 + #13#10 +
           'Next steps:' + #13#10 +
           '  1. Open DAW and rescan VST3 plugins' + #13#10 +
           '  2. Add "AI MIDI Composer" to a MIDI track' + #13#10 +
           '  3. First use downloads the AI model (~4 GB)';
    MsgBox(Msg, mbInformation, MB_OK);
  end;
end;
