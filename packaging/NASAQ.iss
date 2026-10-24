; Sign the payload first, compile, then sign this installer for signed releases.
#ifndef StageDir
  #error StageDir must point to a staged NASAQ payload
#endif
[Setup]
AppId={{1D62081C-2250-4E92-9076-D25B6329D8EF}
AppName=NASAQ — نسق
AppVersion=0.3.12
VersionInfoVersion=0.3.12.0
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
ChangesAssociations=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=NASAQ-0.3.12-setup
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

; Register as a candidate, preserving the Windows user's default choice.
[Registry]
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio"; ValueType: string; ValueName: ""; ValueData: "NASAQ Audio"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\NASAQ.exe,0"
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\NASAQ.exe"" --play --append ""%1"""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "NASAQ"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "NASAQ audio player"
Root: HKCU; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "NASAQ"; ValueData: "Software\Classes\Applications\NASAQ.exe\Capabilities"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.mp3\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mp3"; ValueData: "NASAQ.Audio"
Root: HKCU; Subkey: "Software\Classes\.wav\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".wav"; ValueData: "NASAQ.Audio"
Root: HKCU; Subkey: "Software\Classes\.flac\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".flac"; ValueData: "NASAQ.Audio"
Root: HKCU; Subkey: "Software\Classes\.ogg\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ogg"; ValueData: "NASAQ.Audio"
Root: HKCU; Subkey: "Software\Classes\.m4a\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".m4a"; ValueData: "NASAQ.Audio"
Root: HKCU; Subkey: "Software\Classes\.opus\OpenWithProgids"; ValueType: string; ValueName: "NASAQ.Audio"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities\FileAssociations"; ValueType: string; ValueName: ".opus"; ValueData: "NASAQ.Audio"

; Executable registration enables discovery in the Windows Open With picker.
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "NASAQ — نسق"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\NASAQ.exe,0"
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\NASAQ.exe"" --play --append ""%1"""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\NASAQ.exe,0"
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".mp3"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".wav"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".flac"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".ogg"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".m4a"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\SupportedTypes"; ValueType: string; ValueName: ".opus"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\Application"; ValueType: string; ValueName: "ApplicationName"; ValueData: "NASAQ"
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\Application"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "NASAQ audio player"
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\Application"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\NASAQ.exe,0"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\NASAQ.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\NASAQ.exe"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\NASAQ.Audio\shell"; ValueType: string; ValueName: ""; ValueData: "open"
Root: HKCU; Subkey: "Software\Classes\Applications\NASAQ.exe\shell"; ValueType: string; ValueName: ""; ValueData: "open"
