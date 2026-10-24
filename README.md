# نسق NASAQ

![NASAQ](branding/nasaq-128.png)

**إدارة الصوت والإعلانات المجدولة للأعمال — Commercial Audio Manager for Windows**

## Download / التحميل

**[Download NASAQ from Microsoft Store](https://apps.microsoft.com/detail/9PH1T4418KTF)**

**[تحميل نسق من متجر مايكروسوفت](https://apps.microsoft.com/detail/9PH1T4418KTF?hl=ar&gl=AE)**

NASAQ is available on Microsoft Store, published by **Nawras Jarbouh**. The Store download is free; use requires a separately purchased license activated with a license key. Downloading the application does not include a usage license. Optional Google Cloud speech charges are separate.

التطبيق متاح على Microsoft Store باسم الناشر **Nawras Jarbouh**. التنزيل مجاني، لكن الاستخدام يتطلب ترخيصًا منفصلًا ومفتاح تفعيل. خدمة Google Cloud الصوتية اختيارية ولها تكاليف مستقلة.

## Source version

This source release is **NASAQ 0.3.12** (**0.3.12.0 x64 MSIX**), fixing HTTPS radio runtime packaging while retaining automatic prayer schedules, Store update controls and Windows audio-file associations. See the [0.3.12 release notes](docs/RELEASE-0.3.12.md). Microsoft Store certification and availability are separate from this source release. The package identity is `NawrasJarbouh.NASAQ`; the Store product ID is `9PH1T4418KTF`.

See [Store source provenance](docs/STORE-RELEASE.md) for the archived package hashes, source reconstruction, and verification limits. The 0.3.10 baseline excludes the later license-deactivation button; 0.3.11 includes it. GitHub source updates do not automatically update Microsoft Store; Store updates require a separate package submission.

## Features

- Audio playback, music libraries and playlists, using Strawberry's player and GStreamer.
- Scheduled announcements, microphone paging and coordinated audio priorities.
- Arabic and English interface; installed Windows text-to-speech voices.
- Optional Google Cloud text-to-speech with your own service configuration and credentials.
- Prayer-time playback pauses for the selected city, and optional adhan.
- Operator PIN, schedule recovery, local logs and CSV/PDF reports.
- 16-character Keygen activation, device limits and signed offline allowance of up to seven days.
- MSIX packaging and package-aware startup/update behavior. External GitHub update checks are disabled when running as a Store package.

An application license does not grant music broadcasting or public-performance rights. Users must obtain permissions for the audio they play. Reports and records do not constitute regulatory certification.

## Build

This is a modified **GPL-3.0-or-later fork of [Strawberry Music Player](https://github.com/strawberrymusicplayer/strawberry)**, not an official Strawberry release. Original copyright notices, contributors and license text are retained.

- [Windows build instructions](docs/BUILD.md)
- [Microsoft Store / MSIX packaging](docs/MSIX.md)
- [Release configuration and provenance](docs/STORE-RELEASE.md)
- [Activation setup](docs/ACTIVATION.md)
- [Upstream README](README-UPSTREAM.md), [license](COPYING), [modifications](docs/MODIFICATIONS.md)

The build target is `strawberry.exe`; the distribution executable is named `NASAQ.exe`. NASAQ modules are under `cam/`. Dependencies must be obtained from the documented upstream sources. Do not rename legacy storage identifiers or delete user databases when changing branding.

## Verification and scope

The archived pre-submission regression run recorded **236 passed, 0 failed and 6 skipped checks** across Arabic and English runs of seven suites. This is historical local test evidence, not a new GitHub CI run or a claim of exhaustive installed-package testing. [Store source provenance](docs/STORE-RELEASE.md) includes the recorded suite results.

Microsoft Store availability has been checked on the public product page. Physical audio hardware, long-running venue operation and clean-machine installation coverage remain separate acceptance tasks. Publisher-managed speech credits, subscription billing and Teletext are not implemented.

## Project information

- [Privacy and network behavior](docs/PRIVACY.md)
- [Google Cloud speech setup](docs/GOOGLE-CLOUD-TTS.txt)
- [Third-party inventory](docs/THIRD-PARTY.md)
- [Code signing policy](docs/CODE-SIGNING-POLICY.md)
- Maintainer: [NAWRASJ](https://github.com/NAWRASJ)

This repository publishes source, assets, build scripts and documentation. It does not contain customer license keys, publisher API tokens, private signing keys or personal application databases. Use the Microsoft Store link above for the consumer download.
