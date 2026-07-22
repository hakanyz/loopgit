[Setup]
AppName=LoopGit
AppVersion=1.1.2
AppPublisher=Hakan
AppPublisherURL=https://github.com/hakanyz/loopgit
DefaultDirName={autopf}\LoopGit
DisableProgramGroupPage=yes
OutputBaseFilename=LoopGit_1.1.2_Setup
SetupIconFile=resources\loopgit_radius_icon.ico
UninstallDisplayIcon={app}\LoopGit.exe
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The main executable
Source: "build\LoopGit.exe"; DestDir: "{app}"; Flags: ignoreversion
; The App Icon for shortcuts
Source: "resources\loopgit_radius_icon.ico"; DestDir: "{app}"; Flags: ignoreversion
; The Qt DLLs
Source: "build\*.dll"; DestDir: "{app}"; Flags: ignoreversion
; Qt plugin directories
Source: "build\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\LoopGit"; Filename: "{app}\LoopGit.exe"; IconFilename: "{app}\loopgit_radius_icon.ico"
Name: "{autodesktop}\LoopGit"; Filename: "{app}\LoopGit.exe"; IconFilename: "{app}\loopgit_radius_icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\LoopGit.exe"; Description: "{cm:LaunchProgram,LoopGit}"; Flags: nowait postinstall skipifsilent
