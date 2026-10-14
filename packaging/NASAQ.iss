; Evaluation installer. Sign the payload first, compile, then sign this installer.
#ifndef StageDir
  #error StageDir must point to a staged NASAQ payload
#endif
[Setup]
AppId={{1D62081C-2250-4E92-9076-D25B6329D8EF}
AppName=NASAQ — نسق
AppVersion=0.2.1
DefaultDirName={localappdata}\Programs\NASAQ
DefaultGroupName=NASAQ
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=NASAQ-0.2.1-setup
OutputDir=..\installer
Compression=lzma2
SolidCompression=yes
LicenseFile=..\COPYING
UninstallDisplayIcon={app}\NASAQ.exe
SetupIconFile=..\branding\nasaq.ico

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\NASAQ"; Filename: "{app}\NASAQ.exe"
[Run]
Filename: "{app}\NASAQ.exe"; Description: "Launch NASAQ"; Flags: nowait postinstall skipifsilent
