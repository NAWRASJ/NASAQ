# NASAQ 0.3.19 — Video startup, vinyl display and speech recovery

- Fix local video freezing shortly after startup while waiting for the audio buffer. Playback now continues without an extra play, pause or seek action.
- Correct the vinyl tonearm mounting plate: keep one stationary base, align the pivot and render a clean rotating bearing without overlapping clipped layers.
- Release the announcement hold when Windows SAPI reports a synchronous speech-synthesis failure without a state notification. Reinitialize the provider before retrying.
- Make chime-import verification use an explicit isolated library directory; the application's default storage location remains unchanged.

Includes the local video player, floating media window, optional external audience display, prayer holds and radio recovery from 0.3.18.

Validation: targeted video, audio/chime and radio recovery suites. Real English speech synthesis is unavailable in the restricted test environment, no Arabic SAPI voice is installed, and a second physical display is unavailable. Error recovery is verified separately; these hardware/environment checks are not claimed completed. See verification.json and the included test evidence for the exact release-build results.

The unsigned x64 MSIX is prepared for Microsoft Store submission, not direct signed installation. The Store product is [NASAQ](https://apps.microsoft.com/detail/9PH1T4418KTF), identity `NawrasJarbouh.NASAQ`. Store publication requires Microsoft certification. Preparation of these files does not publish the update to either service.

Corresponding GPL source and SHA256 checksums accompany the package. Credentials, license databases and private signing keys are excluded.
