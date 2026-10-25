# Silent application notifications

NASAQ's routine UI alerts are visual-only. Production message boxes use
SilentMessageBox, which explicitly disables native dialogs before configuring
icons, content or buttons. Information, warning, error and confirmation static
helpers retain their original StandardButton result and default button.

On Windows, OSD native/tray selections (including existing saved settings and
previews) render through the existing Qt OSDPretty widget. They never call the
Windows tray-balloon API. Disabled notifications remain disabled except explicit
previews, as before. Saved preferences are not rewritten.

This does not mute the player, advertisements, microphone, configured announcement
chimes or optional adhan audio. It does not change the Windows sound scheme or
other applications. OS-generated failures before NASAQ can start and external
applications are outside this policy.

Validation: silent_notifications_test checks all message severities, native-dialog
suppression, visual content, buttons, explicit default No and the returned choice.
The test runs offscreen with an isolated Qt test executable; no customer settings
or license data are changed by this test.
