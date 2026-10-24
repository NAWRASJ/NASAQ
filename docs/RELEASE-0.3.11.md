# NASAQ 0.3.11

Microsoft Store download: https://apps.microsoft.com/detail/9PH1T4418KTF

This release prepares MSIX version 0.3.11.0 for Microsoft certification.
Publishing this GitHub release does not mean the Store update is approved or available.

## Changes

- Prayer schedules refresh automatically after the country and city are saved. First-use guidance explains the remaining setup and the separate music-pause preference.
- Store installations check for updates automatically and offer a manual check and a link to update through Microsoft Store.
- Register MP3, WAV, FLAC, OGG, M4A and Opus file associations. A settings button opens Windows Default apps; the user retains control of defaults.
- Open audio files in the existing window of the same installed variant. Files opened during a prayer pause are added without bypassing the pause; saved prayer holds also guard startup playback.
- Add local license deactivation and correct per-user audio-handler registration for the standalone installer.

Publisher-managed Google speech credits, Google OAuth onboarding and payment integration remain deferred and are not part of this release.

## Verification

Recorded local Qt test results: activation 20 passed, review/UI 10 passed, targeted audio 4 passed; no failures in these runs. Full application build and MakeAppx packaging completed successfully.

On Windows, two Explorer double-clicks of Arabic-named WAV files opened in the same standalone 0.3.11 process/window. During a scheduled prayer hold, Windows Shell file activation added a file without a playback-start event; playback resumed after the hold ended. Shell activation was used for the during-hold check after UI automation timed out. Test events and playlist entries were removed afterward.

These checks exercised the standalone installation. Installed Store-package activation and Store update availability still require verification with the Store-distributed package. They are not claims of exhaustive codec, hardware, or clean-machine testing.

## Submission artifact

- Identity: `NawrasJarbouh.NASAQ`
- Version: `0.3.11.0`, architecture: x64
- Unsigned MSIX SHA-256: `439e4b5eb729b0eb532b80256b74950438280e5a4ae41aca4eef9e91e696fe77`
- Packaged NASAQ.exe SHA-256: `7f1da76d60f027080ca2d00a58f785e9ce0a2497eab441bddb13a5846fcf37c5`

The unsigned MSIX is intended for Partner Center submission, not direct consumer installation. Microsoft signs approved Store packages. Use the Store download link above for installation.

The 0.3.10 source provenance remains in [STORE-RELEASE.md](STORE-RELEASE.md).
