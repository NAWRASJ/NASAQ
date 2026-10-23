# NASAQ Midnight — 0.3.0 interface refresh

This working-tree build replaces the main Windows application's legacy fruit artwork and textured navigation with NASAQ branding, a near-black palette, turquoise accents, flat navigation and original line icons. It is a UI preview, not a newly certified or signed release.

## Implementation

- 86 original SVG interface icons, with checked-in PNG renderings at 16, 22, 32, 48, 64 and 128 pixels. Runtime SVG support is not required. The SVG files are the editable source. External service/backend trademarks are not represented as NASAQ-owned artwork.
- Shared Fusion palette and QSS for menus, dialogs, forms, tables, focus, selection, hover and disabled states.
- Repainted horizontal sidebar entries and flat volume control. Existing keyboard and mouse handlers remain in use. Legacy animated character analyzers are removed from the menu; functional audio visualizers remain.
- Music list defaults focus on track, title, artist, album and duration. The first launch backs up the previous column layout in `Playlist/nasaq_previous_columns`; columns remain configurable through the existing header menu.
- Commercial controls grouped by library/scheduling and live operation. STOP is visually distinct, and activation status is prominent. A scrollable central area keeps controls reachable on smaller screens.
- Custom activation dialog with a labelled 16-character key field, network explanation, explicit Activate and Cancel actions. No license enforcement was removed or bypassed.
- Fixed a synchronous PaletteChange/style-sheet re-entry loop exposed by the application-wide theme. The loader now guards each widget's update and avoids reapplying identical CSS.

## Verification and limits

See the delivered `evidence` folder for the current executable hash, XML test results, startup/playback-denial assertions and screenshots from the running application. The UI smoke opens and cancels the activation dialog without submitting a key. Earlier candidate crashes were diagnosed from local Windows crash dumps; those dumps are not included in the delivery.

The public Keygen account configuration is still absent in this UI build. Real-provider activation acceptance, Windows signing, clean-machine installer acceptance and a full screen-reader/RTL/localization audit remain separate work. Not every third-party plugin dialog has been individually reviewed. Native Windows file dialogs follow the system theme. Library and playback engine behavior still derives from Strawberry; this is a branded fork, not a claim of independent authorship of the engine.

## Source and rights

Original Strawberry and other upstream copyright notices, GPL terms and credits are retained. NASAQ's new interface code and artwork are GPL-3.0-or-later. New icon sources live in `data/nasaq`, and the reusable design system in `cam/assets/nasaq.qss`. Historical resource filenames may remain to preserve compatibility; their displayed artwork is replaced.

The source archive contains the modified working tree, including untracked new artwork, plus the dependency sources and build records from the earlier source delivery. The executable's version remains 0.3.0; identify this UI preview by its delivery name and SHA-256.
