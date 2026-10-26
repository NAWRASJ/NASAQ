# NASAQ 0.3.18 — Video playback and floating media display

- Play local video in the existing licensed audio engine and playlist, with a floating window, seek controls and full-screen mode.
- Add an optional external audience display without playback controls. Windows Extend mode and an external screen are required.
- Show a detailed animated vinyl player, genuine album artwork and a live audio visualizer for audio playback, with radio connection indicators.
- Add a configurable 0–5 minute early prayer pause. Optional adhan audio remains at the scheduled prayer time.
- Show a bilingual prayer pause card and resume countdown in the floating window, with a customer image override.
- Improve quiet radio reconnection while respecting prayer holds, manual stop and license state.
- Update media opening labels and video file associations; save display settings atomically and handle storage errors without closing the dialog.

Validation: video playback, pause/seek/resume, prayer holds, image ownership and display settings tests pass. Physical external-monitor/HDMI testing remains pending. Earlier diagnostic tests also flagged English speech completion and chime-import storage behavior in a restricted environment; these are not claimed fixed by this release.

Download from [Microsoft Store](https://apps.microsoft.com/detail/9PH1T4418KTF). Store availability follows Microsoft certification; publishing this GitHub release does not mean the Store update is live.

The unsigned MSIX is for Store submission. Corresponding GPL source and SHA256 checksums accompany this release. Local test certificates, activation data and credentials are excluded.
