# NASAQ 0.3.23

- Keep the playlist total duration permanently visible at the upper-left corner of the daily workspace, including in the Arabic interface.
- Use a compact counter independent of the advanced coverage details setting.
- Preserve the existing duration calculation and live-radio indication.

Validation: build the versioned application, verify the actual window shows the correct playlist total at the left while advanced coverage is hidden, and check playlist drag/drop, undo/redo and timing. Validate MSIX identity, version, executable, bundled codecs, archive integrity and absence of private profile data. This focused UI update does not repeat the full media regression suite from 0.3.22. See verification.json for results.

The unsigned x64 MSIX is intended for Microsoft Store submission. Store availability follows certification and publication. Application and codec sources and SHA256 checksums are included.
