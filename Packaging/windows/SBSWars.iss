; SBS Wars Inno Setup script (Windows)
#define MyAppName "SBS Wars"
#define MyAppVersion "1.0.0"
#define MyAppExeName "SBSWarsLauncher.exe"

[Setup]
AppId={{8F3C2A91-7B44-4E1A-9C12-SBSWARS0001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={localappdata}\SBSWars
DefaultGroupName=SBS Wars
OutputDir=..\..\Dist
OutputBaseFilename=SBSWarsSetup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=lowest
SetupIconFile=sbswars.ico
UninstallDisplayIcon={app}\SBSWarsLauncher.exe
DisableProgramGroupPage=no
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: checkedonce
Name: "startmenu"; Description: "Create a Start Menu shortcut"; GroupDescription: "Shortcuts:"; Flags: checkedonce

[Files]
Source: "..\..\Binaries\sbswars.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Binaries\sbswars-server.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Binaries\sbswars-launcher.exe"; DestDir: "{app}"; DestName: "SBSWarsLauncher.exe"; Flags: ignoreversion
Source: "..\..\Binaries\sbswars-installer.exe"; DestDir: "{app}"; DestName: "SBSWarsInstaller.exe"; Flags: ignoreversion skipifsourcedoesntexist
Source: "sbswars.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\UI\sbswars.png"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\..\Maps\*"; DestDir: "{app}\Maps"; Flags: ignoreversion recursesubdirs
Source: "..\..\Content\*"; DestDir: "{app}\Content"; Flags: ignoreversion recursesubdirs
Source: "..\..\Audio\*"; DestDir: "{app}\Audio"; Flags: ignoreversion recursesubdirs
Source: "..\..\UI\*"; DestDir: "{app}\UI"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\SBS Wars"; Filename: "{app}\SBSWarsLauncher.exe"; IconFilename: "{app}\sbswars.ico"; Tasks: startmenu
Name: "{group}\Uninstall SBS Wars"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\SBS Wars"; Filename: "{app}\SBSWarsLauncher.exe"; IconFilename: "{app}\sbswars.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\SBSWarsLauncher.exe"; Description: "Launch SBS Wars"; Flags: nowait postinstall skipifsilent
