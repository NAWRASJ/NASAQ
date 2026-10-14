# Privacy and network behavior — engineering preview

NASAQ stores schedules, local playback events, operator PIN hashes and music-licence
records/documents on the Windows account's local storage. CSV/PDF exports are written
to the location selected by the user. Local account administrators can modify records.

Activation sends the licence key, public account/product identifiers and a product-specific hashed device fingerprint to api.keygen.sh over HTTPS. Keygen also sees the connection IP. Windows DPAPI protects the cached signed certificate (including the customer key) and last-seen time for the Windows user. The application refreshes at startup and hourly while it has an in-memory key; explicit service rejection invalidates the cache. See ACTIVATION.md for offline expiry and limitations.
Release checking contacts api.github.com only when requested from NASAQ controls;
opening a release page requires confirmation in that flow.

Strawberry-derived functionality may use third-party metadata, covers, lyrics,
scrobbling, radios and configured network services. Their settings and upstream
provider policies apply. This document does not claim the entire application is
offline or that every inherited connection has been audited.

Microphone paging captures audio after the operator arms/uses it. The implemented
SAPI path uses installed Windows voices. SILMA/cloud voice deployment is unfinished.
No analytics service was added by the NASAQ modules. A comprehensive network/privacy
acceptance review remains a release gate. Do not post licence documents or keys in
public issues; no dedicated private support channel is configured yet.
