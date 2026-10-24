# NASAQ 0.3.13 — file activation

Opening an audio file from Windows now reuses its existing entry in the current
playlist. Reopening the currently playing entry does not restart it. New paths
are inserted once, including when repeated requests arrive during metadata loading.
File identity uses the full canonical path (case-insensitive on Windows), not the
display title. Existing intentional duplicates are not deleted automatically.

The Store and desktop associations use `--open-files`. Older associations using
`--play --append` receive the same protection. Plain `--append` still supports
intentional repeats. The existing Playlist > Remove duplicates action remains
available for explicit cleanup.

During a prayer interruption, file activation may add/select an entry, but cannot
start playback or replace the interrupted track. The normal prayer resume behavior
and license/operator playback gates remain in place.

Validation on Windows, 24 October 2026:

- File-activation and selected prayer-interruption Qt tests passed.
- Repeated sequential and concurrent activation requests reused two entries in
  one process/window. A file with the same name in a different folder remained
  a separate entry.
- Requests during a live prayer hold did not start playback; the original track
  resumed when the hold expired.
- The original playlist and preferences were unchanged; activation was retained.

These checks used the release executable before Store signing. End-to-end Explorer
double-click verification against the installed Store update remains pending.
Installing this update requires Microsoft Store publication; an existing 0.3.12
installation does not yet contain this fix.

Download: https://apps.microsoft.com/detail/9PH1T4418KTF
