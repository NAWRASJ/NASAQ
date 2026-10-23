NASAQ 0.3.1 — Audio format update

Built successfully with MSVC/Qt. Added distinct Open audio and Open playlist buttons.
12 synthetic audio formats decoded to nonzero PCM by the packaged GStreamer libraries, isolated from development plugin paths: WAV, MP3, FLAC, Ogg Vorbis, Opus, AAC/M4A, AAC ADTS, AIFF, WMA, AC3, Matroska FLAC, ALAC/M4A. Arabic filenames exercised. Corrupt MP3 correctly rejected.
28 application tests passed. Integrated startup, activation-dialog display and unlicensed playback denial passed. Initial cold smoke exceeded 30 seconds; repeat with 60 second bound passed.
GStreamer command-line and MinGW FFmpeg tools were blocked by Windows application control. A locally compiled source-included diagnostic runner was used instead. Windows protection was not disabled.

LIMITATIONS: Tests verify decoder pipelines, not listening quality or end-to-end licensed playback. Keygen publisher configuration remains absent in this build, so normal playback stays blocked pending configuration and activation. CAM announcement playback uses Qt/Windows and is not covered by the expanded music decoder format matrix. DRM and every possible format/file are not guaranteed. Clean-machine installer/upgrade/uninstall and code signing remain pending.
PUBLIC DISTRIBUTION GATE: Upstream attribution and GPL retained. Complete corresponding source and notices for all binary dependencies still require verification (including GPL-enabled FFmpeg 9.0 and GStreamer 1.28.7). This is a local evaluation package, not a redistribution-ready certified release.
