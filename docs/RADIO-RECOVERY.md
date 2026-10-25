# Radio connection recovery

Live radio errors retain the current playlist item and reconnect after 2, 5, 10, 20, 30 and then 60 seconds. Only one retry timer exists. Five seconds of advancing playback resets the backoff. A live stream ending also reconnects rather than advancing to an unrelated track; explicit stop-after is respected.

The player shows a quiet Arabic/English reconnecting status. Radio transport errors do not emit the modal Player error signal. File playback errors retain their existing reporting. Errors from obsolete GStreamer pipelines are ignored at the engine/player boundary.

Manual Stop/Pause cancels retries. Changing the playing item resets recovery; a removed item or changed active playlist cannot be resurrected by a queued retry. Every retry passes the normal licence and prayer/foreground-audio gates. A prayer hold cancels recovery, and CAM owns the later resume decision. Stopping manually during a hold prevents CAM from restarting the old radio item. Late URL-handler results also check the hold before playback. HTTP inactivity is bounded by a 15-second source timeout; TLS validation is unchanged.

Verification: `nasaq_radiorecovery_tests` covers repeated errors, backoff, stable recovery, manual cancellation, item changes/removal, licence/startup/prayer holds, EOF, ordinary file errors and late asynchronous URL results. Its real HTTP test uses the production Player/GStreamer path with a silent fakesink: 503 outage, recovery, forced connection loss, prayer hold, explicit resume and final stop. Windows tests need GLib event processing and permission to connect to their loopback HTTP server.

Regression suites: `cam_business_tests`, `cam_audio_tests`, `nasaq_fileactivation_tests`. The final run had no failures; two existing audio cases were skipped because the optional MP3 fixture and Arabic SAPI voice were unavailable. This does not imply testing every station's future availability or an installed Store update.
