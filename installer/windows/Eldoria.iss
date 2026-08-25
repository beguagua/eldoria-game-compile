[Setup]
AppName=Eldoria
AppVersion=0.1.0
DefaultDirName={autopf}\Eldoria
DefaultGroupName=Eldoria
OutputDir=installer-output
OutputBaseFilename=Eldoria-Windows-Installer
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayName=Eldoria

[Files]
Source: "..\..\build\windows\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Eldoria"; Filename: "{app}\Eldoria.exe"
Name: "{autodesktop}\Eldoria"; Filename: "{app}\Eldoria.exe"

[Run]
Filename: "{app}\Eldoria.exe"; Description: "Executar Eldoria"; Flags: nowait postinstall skipifsilent
