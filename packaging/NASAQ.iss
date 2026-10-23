; Sign the payload first, compile, then sign this installer for signed releases.
#ifndef StageDir
  #error StageDir must point to a staged NASAQ payload
#endif
[Setup]
AppId={{1D62081C-2250-4E92-9076-D25B6329D8EF}
AppName=NASAQ — نسق
AppVersion=0.3.10
VersionInfoVersion=0.3.10.0
WizardStyle=modern dynamic
DisableWelcomePage=no
DisableDirPage=no
DisableProgramGroupPage=no
AllowNoIcons=yes
ShowLanguageDialog=yes
CloseApplications=yes
RestartApplications=no
DefaultDirName={localappdata}\Programs\NASAQ
DefaultGroupName=NASAQ
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=NASAQ-0.3.10-setup
OutputDir=..\installer
Compression=lzma2/fast
SolidCompression=yes
LicenseFile=..\COPYING
UninstallDisplayIcon={app}\NASAQ.exe
SetupIconFile=..\branding\nasaq.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"

[CustomMessages]
english.DesktopShortcut=Create a desktop shortcut
arabic.DesktopShortcut=إنشاء اختصار على سطح المكتب
english.Shortcuts=Shortcuts:
arabic.Shortcuts=الاختصارات:
english.LaunchApp=Launch NASAQ
arabic.LaunchApp=تشغيل نسق

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; GroupDescription: "{cm:Shortcuts}"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "tests\*,nasaq_codec_probe.exe,NASAQ-audit.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\NASAQ"; Filename: "{app}\NASAQ.exe"
Name: "{autodesktop}\NASAQ"; Filename: "{app}\NASAQ.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\NASAQ.exe"; Description: "{cm:LaunchApp}"; Flags: nowait postinstall skipifsilent
