# NASAQ 0.2.1 dependency inventory and release gate

This file supersedes THIRD-PARTY.md for the integrated 0.2 build. The older document
describes only the separate 0.1 MinGW prototype.

* Strawberry 1.2.31, commit f16e11e7c0eb61aebca2bd7897700ecab8a1350d: GPL-3.0-or-later.
  Complete modified application source and NASAQ source are in NASAQ-0.2.1-source.zip.
  It is a derivative engineering preview, not an official Strawberry release.
* Official strawberry-msvc-dependencies release 0.1.321, SHA256
  02f48098e876deabb6770bea60d4059ee7c92f0f768d8d03eff78d5cec9ab069.
  Source/build provenance: https://github.com/strawberrymusicplayer/strawberry-msvc-dependencies
  and https://github.com/strawberrymusicplayer/strawberry-msvc .
* Qt 6.12.0 shared libraries: LGPL/GPL and component-specific third-party terms.
  Qt Multimedia, Speech and ShaderTools were built from their exact v6.12.0 tags;
  matching module source and LICENSES are supplied in the source archive.
  Qt Base/ImageFormats/Tools provenance is in the supplied SPDX manifests.
* GStreamer, GLib, TagLib, libebur128, codec libraries, SQLite, OpenSSL, ICU and
  dependencies come from the official MSVC bundle. Its retained notices and Qt SPDX
  manifests are an inventory input, not a complete NASAQ SBOM or certification.
* Microsoft VC runtime DLLs are vendor binaries, subject to Microsoft redistribution
  terms. Windows SAPI and Media Foundation are OS services.
* Inno Setup 6.7.3 produced an evaluation installer. Check the compiler vendor's
  commercial licensing requirements before commercial release.

PUBLIC DISTRIBUTION GATE: collect and verify corresponding sources/build recipes and
all applicable notices for every redistributed dependency, including Qt Base and
GStreamer/GLib. This delivery does not claim that the entire dependency-source set
has been supplied. Do not treat the old 6.8.3 source bundle as matching Qt 6.12.0.
Preserve GPL rights, source availability and replacement/relinking rights as applicable.
The application activation adapter must not be presented as revoking GPL rights.
No streaming credentials, paid voices, SILMA weights or signing private keys are bundled.
