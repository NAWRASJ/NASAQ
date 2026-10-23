# NASAQ form design rules

Forms must communicate the type and expected size of a value. Window width is not a reason to stretch a two-digit number across the screen.

- Numeric controls use their value range, suffix and font metrics to reserve enough space for the complete value and step buttons.
- Date and time editors use their content size; values remain left-to-right in both languages. Labels and form order follow the interface language.
- Related date endpoints share a row, as do daily start/end times. The labels distinguish the date range from the daily operating window.
- Choice fields fit their options up to a readable maximum; long text and announcement editors retain flexible space.
- Single-line text fields have a moderate maximum width. Multi-line text editors and data tables can expand.
- Form labels can wrap and narrow windows wrap long rows. The schedule remains scrollable instead of clipping controls.
- Languages and broadcast order, repetition, and the active date/time window are separate groups. Weekday choices use two short rows, not seven widely separated controls.
- The language and repetition groups share a row above 900 logical pixels of available width; narrower windows stack them vertically. The date/time group stays below them.
- Spacing is consistent: 12 logical pixels between form rows, 20 between labels and fields, 16 between schedule groups. Qt font metrics and logical coordinates support Windows display scaling.

`formdesign.h` applies these rules to the announcement studio, cloud speech, prayer settings, chimes, one-off scheduling and music-rights forms. The activation dialog is already a dedicated short-key/status design and remains unchanged.

Validation includes the studio's data/save/schedule regression tests and geometry checks at 820×680 and 1180×820 logical pixels, in Arabic and English. Screenshots are reviewed in addition to assertions. These checks do not claim exhaustive coverage of every Windows scaling/font combination.
