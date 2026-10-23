# Privacy and network behavior — engineering preview

NASAQ stores schedules, local playback events, operator PIN hashes and music-licence
records/documents on the Windows account's local storage. CSV/PDF exports are written
to the location selected by the user. Local account administrators can modify records.

Activation sends the licence key, public account/product identifiers and a product-specific hashed device fingerprint to api.keygen.sh over HTTPS. Keygen also sees the connection IP. Windows DPAPI protects the cached signed certificate (including the customer key) and last-seen time for the Windows user. The application refreshes at startup and hourly while it has an in-memory key; explicit service rejection invalidates the cache. See ACTIVATION.md for offline expiry and limitations.
Release checking contacts api.github.com only when requested from NASAQ controls;
opening a release page requires confirmation in that flow. This release-check
feature is disabled for MSIX/package-identity and Store builds; their application
updates come through Microsoft Store.

Strawberry-derived functionality may use third-party metadata, covers, lyrics,
scrobbling, radios and configured network services. Their settings and upstream
provider policies apply. This document does not claim the entire application is
offline or that every inherited connection has been audited.

Microphone paging captures audio after the operator arms/uses it. The implemented
SAPI path uses installed Windows voices. Optional Google Cloud synthesis sends
the entered text, language/voice selection and project identifier to
texttospeech.googleapis.com only after the operator permits sending it and supplies
a temporary access token. The token remains in the dialog/request lifetime and is
not saved as a preference. Generated audio is cached in the user's local data
directory; cached reuse does not send another request. Microphone paging audio is
not sent to Google Cloud by this implementation.
No analytics service was added by the NASAQ modules. A comprehensive network/privacy
acceptance review remains a release gate. Do not post licence documents or keys in
public issues; no dedicated private support channel is configured yet.
Prayer-time synchronization (0.3.4): when enabled, NASAQ sends the selected city, country and calculation-method number to api.aladhan.com over HTTPS. The provider sees the connection IP. It fetches the current monthly calendar, converts explicit offsets to UTC, stores future events locally, and retries hourly until one successful refresh per local day while the app is open. This is one API provider offering several calculation authorities, not direct connections to each authority. A failed fetch preserves the previous schedule. Disabling automatic prayer scheduling removes only pending API-generated events; manual imports and history are preserved. No audio files are uploaded. Local mosque times should be checked before enabling automatic adhan.
