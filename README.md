# نسق NASAQ

![NASAQ](branding/nasaq-128.png)

**إدارة الصوت التجاري — Commercial Audio Manager**

NASAQ is a modified, GPL-3.0-or-later fork of [Strawberry Music Player](https://github.com/strawberrymusicplayer/strawberry), not an official Strawberry release. It combines Strawberry's music library/player with commercial scheduling and audio controls. Original copyright notices, contributors and GPL text are retained. See [upstream README](README-UPSTREAM.md), [COPYING](COPYING), and [modifications](docs/MODIFICATIONS.md).

## Current status

Engineering preview **0.2.1**, unsigned. Windows source builds and local tests have passed; this is not a production-ready or certified product. No SignPath acceptance or trusted signing has been obtained. See the [Code signing policy](docs/CODE-SIGNING-POLICY.md) and [readiness assessment](docs/SIGNPATH-READINESS.md).

## Features and limits

* Music playlists use Strawberry's in-process Player/PlaylistManager and GStreamer.
* One-shot schedules, imported prayer timetables, optional adhan and announcements.
* Installed SAPI voices, microphone paging and coordinated audio priorities.
* Local operator PIN, schedule recovery, music-licence document records and renewal reminders.
* Local playback logs and CSV/PDF export; these do not grant music rights or certify compliance.
* Keygen validation adapter and user-triggered GitHub release checks.

Recurring schedules/editing, automatic prayer calculation, SILMA/cloud voices, complete activation/offline policy, long-running recovery and hardware/install acceptance remain unfinished. Teletext is deferred.

## Build and use

See [Windows build instructions](docs/BUILD.md). Source is in the upstream layout, with NASAQ modules in `cam/`. Brand assets are in `branding/`. The upstream build target is `strawberry.exe`; the branded distribution uses `NASAQ.exe`.

Open **نسق NASAQ / إدارة الصوت** in the player. Legacy internal storage identifiers `CAMProject`, `CAM-Strawberry`, `cam.sqlite`, and the `CAM_SMOKE_OUTPUT`/`CAM_TEST_WAV` test variables remain for compatibility. Do not rename or delete user databases when changing branding.

## Verification

The local MSVC/Qt build passed 7 core and 8 audio functional tests. The integrated silent-WAV smoke passed playlist loading, playback, suspension, blocked manual resume, resume and stop. It also passed with development tool directories removed from PATH. These results are local evidence, not GitHub CI results or clean-machine certification. Installer compilation succeeded; install/upgrade/uninstall and physical PA acceptance remain pending.

## Project information

* [Privacy and network behavior](docs/PRIVACY.md)
* [Third-party inventory and release gates](docs/THIRD-PARTY.md)
* [Code signing policy](docs/CODE-SIGNING-POLICY.md)
* Repository owner: [NAWRASJ](https://github.com/NAWRASJ)

The repository includes source, brand assets and documentation. No private keys, activation credentials, personal database, or Windows installer is intentionally included. Dependencies must be obtained from their documented upstream sources; the dependency-source compliance checklist is not yet complete.
