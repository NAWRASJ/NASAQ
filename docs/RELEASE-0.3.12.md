# NASAQ 0.3.12

Fixes radio streams failing with “Missing GStreamer plugin for HTTPS protocol source”.
The Windows runtime now includes the Soup HTTP/HTTPS source, ICY metadata demuxer,
TLS module and their transitive DLL dependencies from the same dependency build.

Packaging rejects runtimes missing these components. `stage-radio-runtime.ps1`
checks dependencies and rejects conflicting DLL versions; `test-radio-runtime.ps1`
decodes an HTTPS radio stream with an isolated plugin registry and a restricted
PATH so developer-installed codecs cannot hide missing package files.

The application retains the existing profile identity, activation data, prayer
settings and playlists. No database migration or license reset is introduced.

Download: https://apps.microsoft.com/detail/9PH1T4418KTF

Acceptance checks on Windows, 24 October 2026:

- MP3 and AAC HTTPS streams decoded with isolated staged libraries.
- The release executable played the Doomed station, paused for a scheduled prayer,
  and resumed the same stream when the pause expired.
- File activation using the registered `--play --append` arguments appended a WAV
  during the pause without playback. A second WAV played after the pause; both
  requests reused the original process and window.
- 20 license checks, 10 selected prayer/settings UI checks and 4 selected audio
  checks passed. Test profiles are separate from the user's activation.
- The live test used the release payload before Store signing. Windows' default
  player choice was not changed; Store certification is a separate step.
