# Microsoft Store / MSIX target

The original `_msix-kit` is integrated under `packaging/msix`, including its
manifest, icon generation, resource-index creation and optional test signing.
Inno Setup and `scripts/sign-release.ps1` remain independent supported targets.

## Build and identity

1. Build with `scripts/build-strawberry.ps1 ... -StoreBuild` (or configure
   `-DNASAQ_STORE_BUILD=ON`). Runtime package identity also disables external
   updates, even when packaging a normal build.
2. Stage the complete runtime as for Inno, naming the executable `NASAQ.exe`.
3. Fill `packaging/msix/store-identity.json` with the three **public** values from
   Partner Center > Product identity. Placeholder identities are rejected.
4. Run:

```powershell
./scripts/build-msix.ps1 -StageDir C:/stage/NASAQ -OutDir C:/releases/NASAQ-msix
```

The version comes from `cmake/NasaqVersion.cmake`, also consumed by both CMake
projects. `0.3.10` becomes `0.3.10.0`; a fourth component in the source version
is rejected, as are components greater than 65535. The upstream Strawberry
source version remains separately documented in `cmake/Version.cmake`.

The output is unsigned for Store submission. It is **not** directly installable
as a trusted production package. Store signing occurs after certification;
successful MakeAppx validation does not prove Store acceptance or SAC compatibility.
Full trust (`runFullTrust`) requires justification during submission. Explain
desktop music playback, local libraries, microphone paging and scheduled audio.
Include the GPL source offer/notices and an accurate privacy-policy URL.

`-TestSign` explicitly creates a separate `_LOCAL-TEST.msix` and public `.cer`.
It does not install the certificate or change Windows trust. A self-signed test
certificate is not a substitute for Store trust or a way around Smart App Control.
Never upload the local-test package. Do not commit private certificates/keys.

## Runtime layout

`scripts/normalize-runtime.ps1 -StageDir ...` is shared with Inno staging and
must run **before signing**. The MSIX builder invokes it on its private copy.
It compares hashes before deleting redundant root plugin files and fails on
conflicts. Core DLLs stay beside NASAQ.exe; plugins stay in `platforms/`,
`imageformats/`, `multimedia/`, `tls/`, `gstreamer-plugins/`, etc.

Cause of the old duplicates: the historical `CopyDLLDependencies.ps1` seeds all
EXE/DLLs recursively and copies every seed to DestDir, flattening deployed
plugins. It was not windeployqt itself. The local dependency copier now avoids
recopying files already inside the destination tree; normalization also handles
older stages. The original working runtime is not modified by the MSIX builder.

`qt.conf` sets Prefix and Plugins to `.` relative to the executable. GStreamer
scanner and plugin variables (including `_1_0` variants) are set from the
executable directory. The registry lives in AppLocalDataLocation, whose directory
is created before GStreamer initializes. No registry cache is stored in WindowsApps.

## Writable-path audit

| Data | Destination / behaviour |
| --- | --- |
| Player database | StandardPaths wrapper -> QStandardPaths AppLocalDataLocation |
| CAM database/schedules/log records | AppLocalDataLocation/CAM/cam.sqlite |
| Keygen certificate and DPAPI sealed cache | CAM database settings; no install-folder file |
| Imported chimes | AppLocalDataLocation/chimes |
| Built-in chimes | Read-only executable-relative chimes directory |
| TTS outputs/cache | AppLocalDataLocation/announcements/google |
| Announcement library | AppLocalDataLocation/announcements/library |
| Attached rights documents | AppLocalDataLocation/documents |
| CSV/PDF/audio exports | User-selected save paths (Documents suggested) |
| Player artwork/cache | StandardPaths/QStandardPaths cache paths |
| Settings | QSettings user scope; package virtualization applies |

No portable-mode or existing HKCU Run startup implementation was found in the
application sources. The new explicit Windows startup action uses StartupTask
NASAQStartup for packaged runs; unpackaged runs use HKCU Run. It never enables
startup silently and reports disabled-by-user/policy states. WinRT requests run
on a worker thread. Reopening settings does not automatically override OS choices.
Package identity detection uses GetCurrentPackageFullName; API failure is not
mistaken for a package identity.

Packaged builds hide and guard the GitHub update action. Upstream Sparkle updater
creation had already been removed from MainWindow. The optional Store build also removes Sparkle integration at configuration time.
No external updater is started. Runtime identity separation also prevents a packaged process activating
an already-running unpackaged process through the single-instance socket.

Windows microphone privacy denial is checked before ducking or opening audio.
Microphone failure releases ducking/paging, restores normal playback subject to
prayer/licence state, and displays guidance to check Windows microphone privacy
and device availability. Users remain in control of OS permissions. The microphone
capability is declared. Real permission denial and device-unplug scenarios still
require installed-package testing; a manifest capability alone does not prove access.

## Migration and acceptance checks

MSIX AppData/registry virtualization may give the package a separate first-run
profile. Do not copy DPAPI/activation material automatically or assume an existing
unpackaged activation is reused. Activate the installed package explicitly and
check device limits. Uninstall may remove package-owned data: export important
user-managed content beforehand. Audio files outside the package are not deleted.

Before public delivery, test a Store-signed/private-flight package on Windows 11:
first launch and activation; denied mic permission and restoration; startup enable,
disable, and disable-by-user in Windows Settings; playback of all shipped codecs;
adhan pause/resume; schedules/TTS exports; reopen persistence; Store update with
data retained; uninstall/reinstall. These cannot be inferred from pack success.

References:
- https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/package-version-numbering
- https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.startuptask
- https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes
