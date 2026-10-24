# Audio file activation

The MSIX manifest and Inno Setup installer register MP3, WAV, FLAC, OGG, M4A
and Opus as supported audio files. Registration does not overwrite Windows'
default selection. Settings > Make NASAQ the default audio player opens the
Windows Default apps page, targeting the current package AUMID or the per-user
NASAQ registration where supported.

The open command quotes the filename and supplies `--play --append`. The existing
KDSingleApplication channel forwards it to the running instance of the same
installation type and Windows user. File activation reveals that player window.
Store and unpackaged instances intentionally retain separate IPC identities.

During a CAM interruption, file import appends without requesting playback or
clearing the interrupted playlist. Existing PlayAt/UnPause suspension guards
remain in effect. A saved prayer hold also gates these methods immediately on
startup, before the first scheduler tick. That temporary guard expires by its
timestamp and is cleared when the audio controller takes over suspension.
Files added during a pause are not promised immediate playback when it expires;
the normal interrupted playback resumes according to existing recovery rules.

Local validation: Microsoft MakeAppx schema packaging, Inno Setup compilation,
compilation of the changed player/integration sources, UI tests, and an audio
test opening an Arabic-named WAV while prayer suspension remains active.
Test packages are incomplete validation fixtures, not distributable installers.

Installed-build acceptance remains necessary: install the new complete package,
select NASAQ in Default apps, double-click files with spaces and Arabic names
with the app closed/running/minimized, and repeat during a prayer hold. Confirm
one player window/process for the same installed variant and no audible bypass.
Local component tests do not replace this Windows shell integration check.

Microsoft references:
- https://learn.microsoft.com/en-us/windows/apps/develop/launch/launch-default-apps-settings
- https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/desktop-to-uwp-extensions
