# NASAQ modifications and attribution

Base: Strawberry commit `f16e11e7c0eb61aebca2bd7897700ecab8a1350d` (1.2.31).
Modification date: 2026-10-14. NASAQ engineering preview: 0.2.1.

NASAQ adds `cam/` with local storage, scheduling, audio arbitration, PIN controls,
compliance records, export and activation/update adapters. The in-process adapter
uses Strawberry's existing music player. Player guards prevent manual playback
during NASAQ foreground audio and operator locking.

Window/menu text, translations, Windows version resources, icons and About identify
NASAQ. The original authors and licences remain credited. The logo is original
geometric artwork: three vertical turquoise bands suggesting lowercase n.
Upstream updater prompts are removed from this derivative; NASAQ release checking
is user-triggered and uses the configured project repository.

The GitHub fork preserves upstream history. The initial independently-created
repository remains at https://github.com/NAWRASJ/NASAQ-initial-20261014 .
This publication does not claim Strawberry endorsement or SignPath approval.

0.3.0 adds publisher-side 16-character key issuance, per-key term/device limits, client activation, signed offline certificates, DPAPI storage, and playback gating. Configuration/live-provider acceptance remain outstanding; see ACTIVATION.md.

The Midnight UI refresh replaces visible fruit artwork, adds NASAQ's black/turquoise design system and 86 original vector icons, redesigns navigation and activation, and fixes palette/style re-entry. See MIDNIGHT-UI.md for details and limits.

0.3.10 keeps activation confirmation visible, displays verified license expiry separately from offline grace, reopens active licenses as a read-only status view, and forces the Windows GUI executable subsystem to avoid a console window. Regression tests cover successful activation/reopen, invalid keys, and network failure during cached grace.
