# Selected radio stations

NASAQ now opens Radios on a Selected stations tab. It contains 18 stations, with
Teletext first, and filters for musical style and venue. Venue labels are mood
suggestions, not commercial music licences or guarantees about every broadcast.

Features:
- Arabic/English labels, live language changes, search, and an empty-results state.
- Classical/piano repertoire, jazz, bossa/island rhythms, lounge, chill, ambient,
  acoustic/folk, world music and mellow vocals.
- Offices, restaurants, cafes, shops, hotels and spa/relaxation filters.
- Favorites stored under RadioDiscovery/favorites in the existing preferences.
- Play station, add without playing, and a link to each station's official site.
- Repeated selected-station actions reuse its exact URL in the current playlist.
- Play requests during CAM prayer/announcement suspension only add a missing
  station; they do not replace the interrupted track or release the suspension.
- Existing Channels and Radio Browser remain available.
- Radio seek/scan offsets are ignored; resuming reconnects at the live edge.
- CAM suspension enforces zero business gain even during buffering. Radio connections
  are stopped without fade-out, and the original playlist item is reconnected after
  the hold. A station removed during the hold is not replaced with an unrelated row.

Catalogue: data/radios/curated.json. Each station includes its official source,
homepage, direct stream URL, stable ID and Arabic/English description.

Teletext source: https://radio.teletext-ar.tech/api/nowplaying/teletext
Teletext player: https://radio.teletext-ar.tech/public/teletext
Other sources: https://somafm.com/channels.json,
https://www.radioswissclassic.ch/en/reception/internet,
https://www.radioswissjazz.ch/en/reception/internet,
https://veniceclassicradio.eu/.

Verification on 2026-10-25:
- Release-mode application and nasaq_radio_tests build succeeded.
- Discovery/filter/favorite persistence, explicit action flags, Arabic changes pass.
- All 18 streams decoded audio into a silent sink using the 0.3.13 staged GStreamer
  runtime. Transient SomaFM failures required choosing published alternate servers
  for Groove Salad, Beat Blender and Suburbs of Goa. Future uptime is not guaranteed.
- Existing isolated audio regressions for opening during prayer, license protection,
  and late-prayer duration pass. These use a fake transport, not the installed UI.
- Installation and live playback validation are recorded separately with release QA.
- Corrected-runtime live tests stopped Teletext during buffering for 90 seconds,
  blocked a repeated play request, and reconnected the same station on expiry.
  A second 90-second hold blocked opening another WAV twice and resumed the original
  WAV. The QA playlist retained exactly one entry per source.
- Installed 0.3.16 repeated these checks: Explorer double-clicks reused one process
  and window without duplicates; a 90-second adhan hold stopped buffering Teletext,
  blocked a double-clicked WAV, then reconnected Teletext. No audio errors were logged.
  Original playlist items and settings were preserved; temporary QA data was removed.

Run the widget test with Qt Test on PATH and QT_PLUGIN_PATH pointing to the complete
Qt build plugins (including platforms/qoffscreen.dll and sqldrivers). The packaged
runtime deliberately has only the Windows platform plugin, so it cannot itself run
an offscreen widget test. Use -platform offscreen -o results.xml,xml.
