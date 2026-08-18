; SBS Wars Inno Setup script (Windows)
#define MyAppName "SBS Wars"
#define MyAppVersion "1.0.0"
#define MyAppExeName "SBSWarsLauncher.exe"

[Setup]
AppId={{8F3C2A91-7B44-4E1A-9C12-SBSWARS0001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\SBSWars
DefaultGroupName=SBS Wars
OutputDir=..\..\Dist
OutputBaseFilename=SBSWarsSetup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=lowest

[Files]
Source: "..\..\Binaries\sbswars.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Binaries\sbswars-server.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Binaries\sbswars-launcher.exe"; DestDir: "{app}"; DestName: "SBSWarsLauncher.exe"; Flags: ignoreversion
Source: "..\..\Maps\*"; DestDir: "{app}\Maps"; Flags: ignoreversion recursesubdirs
Source: "..\..\Content\*"; DestDir: "{app}\Content"; Flags: ignoreversion recursesubdirs
Source: "..\..\Audio\*"; DestDir: "{app}\Audio"; Flags: ignoreversion recursesubdirs
Source: "..\..\UI\*"; DestDir: "{app}\UI"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\SBS Wars"; Filename: "{app}\SBSWarsLauncher.exe"
Name: "{autodesktop}\SBS Wars"; Filename: "{app}\SBSWarsLauncher.exe"

[Run]
Filename: "{app}\SBSWarsLauncher.exe"; Description: "Launch SBS Wars"; Flags: nowait postinstall skipifsilent
