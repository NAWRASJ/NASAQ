# NASAQ 0.3.21

## Playback and prayer schedules

- Resume a local file even when an asynchronous pause transition is still pending.
- Keep player state consistent when a late playback notification triggers another pause during a prayer hold.
- Retry the playback handoff when foreground audio state settles after prayer expiry. Log whether previous playback is pending and when resume is requested.
- Retry temporary prayer-service failures, retain the saved timetable on failure, and request the next month when the current calendar has no future prayers.
- Clear a stale prayer-update error after a successful refresh.
- Use a prayer-screen message that covers both audio and video playback.

## Local data management

- Export and restore encrypted local backups from Settings. Backups are tied to the same Windows user and computer.
- Include database snapshots and application preferences; external media files are not copied.
- Queue restore/reset operations for next startup with recovery and rollback protection.
- Preserve the current activation by default during data reset or restoration, and create a recovery backup before destructive operations.

Microsoft Store uninstall is managed by Windows. Export a backup before uninstalling when data retention is required; this release does not add a custom Store uninstall wizard.

## Validation and limits

Prayer tests cover temporary service failure, retained timetables, month rollover and a live AlAdhan request. Playback tests exercise the production transport, local WAV/video playback and a local HTTP radio stream with GStreamer, including delayed state changes, prayer holds, recovery and manual stops. Video tests verify the actual prayer screen, mute gain, preserved playback position and resumed frames, both with and without adhan audio. Audio output in engine tests uses a silent sink. These tests do not prove uninterrupted operation of every remote radio station or reproduce every condition on the user's device.

GitHub publication and Microsoft Store certification are independent. See the release verification and publication-status artifacts for the actual status.
