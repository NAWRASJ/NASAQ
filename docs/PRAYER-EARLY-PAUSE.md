# Early prayer music pause

The prayer settings dialog provides a 0–5 minute lead (`prayer_lead_minutes`, default 0). The next-pause banner uses the adjusted time and displays an hours/minutes/seconds countdown.

Original timetable timestamps and optional adhan audio are unchanged. A separately claimed silent prayer event starts the hold early. Its timestamp remains the original prayer time so the existing post-adhan duration and persisted hold end remain unchanged. For example, 12:06 adhan + 2 minute lead + 10 minute pause means music stops at 12:04 and can resume at 12:16.

The lead is bounded at both the UI and scheduler, applies to imported and online prayer events, and is disabled with automatic prayer pauses. The existing audio hold also blocks file activation, radio recovery and announcements. Synthetic holds are claimed once and persist across restarts. No installed user profile or licensing data is modified during development tests.

This setting compensates for a local mosque starting earlier than the saved timetable; it does not correct the underlying timetable source, city, calculation method or system clock.
