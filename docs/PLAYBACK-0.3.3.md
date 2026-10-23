NASAQ 0.3.3 — Playback restored

Root cause: previous release had empty public Keygen publisher configuration. The license gate rejected playback, independently of decoding support.
Fix: configured the supplied account, product and Ed25519 public key. Release build now requires nonempty public activation configuration. No admin token or customer license key is embedded. Activation gates and signature checks remain enforced.
Live verification: the user-provided test license activated this Windows device through Keygen; signed offline certificate accepted and persisted using Windows account protection. Two real MP3 files from D:/songs played in the actual application, reaching 11.210 and 11.090 seconds with licensed=true and playing=true, exit=0. Second launch reused persisted activation. Six real MP3 files decoded completely without errors using packaged libraries. Original songs were not modified or uploaded.
28 regression tests passed (7 core, 9 audio, 12 licensing). Prior language and codec evidence retained as historical baseline, not newly claimed here.
Remaining: installer is unsigned; clean-machine installation and physical listening quality were not verified. All-format/DRM support is not guaranteed. Some inherited advanced UI strings still require Arabic translation. Complete dependency corresponding-source/notice review remains a public-distribution gate.
