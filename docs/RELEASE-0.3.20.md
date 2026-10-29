# NASAQ 0.3.20 — Expanded local media support

This release extends 0.3.19. GitHub publication and Microsoft Store certification are independent.

- Add local video recognition and file-picker support for WMV/ASF, FLV, MPEG/MPG/MPE, TS/MTS/M2TS, OGV and 3GP/3G2.
- Add audio picker/library support for E-AC-3, CAF and AU/SND.
- Use one local-video classifier for playback routing and presentation so the buffering fix also applies to newly supported containers.
- Bundle missing AVI, FLV and MPEG demuxers, video parsers, AV1 and Theora decoders, AU parsing and CAF demuxing.
- Request accurate seeking for local video containers, including MPEG-TS; verify backwards and forwards jumps.
- Fix AU/SND seeking: preserve open-ended segment stops and account for the file-header offset. Unrecognized optional metadata no longer produces an error-level log.
- Fail packaging when required media plugins are missing.

Runtime: GStreamer 1.28.8, libaom 3.15.2 and a native MSVC Theora 1.2.0 decoder. GStreamer plugins are built against the same dependency prefix as the app. libaom uses its public C API; its import library distinguishes exported data and functions.

Validation details are in the generated format-test report. Tests use synthesized, unencrypted fixtures and the production GstEngine, with a silent audio sink. Decoder success is not a claim of support for every possible codec/container combination, DRM, every damaged file, or unlimited resolution/performance. Existing APE, TAK, DSD and tracker formats are not certified by this matrix.

## Rebuilding media dependencies

From a Visual Studio developer shell with Python, Meson and Ninja available:

```powershell
python scripts/build-format-runtime.py --dependency-prefix <app-msvc-prefix> --ucrt-prefix <tested-ucrt-prefix> --source-archives <archives> --build-dir <new-build-directory> --output <format-bundle>
./scripts/stage-video-runtime.ps1 -StageDir <app-stage> -DependencyPrefix <app-msvc-prefix> -FormatPluginsDir <format-bundle>
```

`packaging/media-codecs/sources.json` pins source archive hashes. `source-patches.json` records checked upstream-source changes: AU segment/seek handling, CAF mapping/ranking, decoder-only AOM registration, MSVC-compatible capability declarations, a public AOM decoder accessor, and useful initialization errors. Theora is built decoder-only from original sources by `build-theora-decoder.py`. Licensing files accompany the generated bundle.

Upstream sources: https://gstreamer.freedesktop.org/src/ and https://github.com/xiph/theora/tree/v1.2.0 . libaom and winpthreads are the existing UCRT dependency distribution, version and file hashes recorded with the bundle.

Generate the basic fixtures and run end-to-end decoding:

```powershell
python scripts/test-media-formats.py --ffmpeg <ffmpeg.exe> --runtime <app-stage-with-codec-probe> --fixtures <fixture-directory> --output <results-directory>
```

`nasaq_video_tests mediaFormatPlayback` additionally exercises startup, frames, pause, resume, seek and stop. Set `NASAQ_FORMAT_FIXTURE` to a seven-second media file and `NASAQ_FORMAT_VIDEO` to 1 when video frames are expected. `invalidMediaFailsCleanly` checks rejection of a corrupt MP4.
