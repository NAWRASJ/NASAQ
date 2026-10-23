# NASAQ 0.3.10 Store source baseline

Download: https://apps.microsoft.com/detail/9PH1T4418KTF

- Publisher: Nawras Jarbouh
- Store product ID: `9PH1T4418KTF`
- Package identity: `NawrasJarbouh.NASAQ`
- Source version: `0.3.10`; MSIX version: `0.3.10.0`; architecture: x64

## Provenance

This baseline was reconstructed on 2026-10-23 from the retained local release
source and the 2026-10-17 source archive. The source archive predates the final
MSIX compatibility changes, so those changes are included from the retained
build tree. The October 17 build logs record the final MSIX compatibility build.
The later local deactivation-button implementation and its tests were excluded
by restoring these files byte-for-byte from the October 17 source archive:

- `cam/src/licenseactivation.cpp`
- `cam/src/licenseactivation.h`
- `cam/src/activationdialog.h`
- `cam/tests/license_test.cpp`

The release tree's source-file timestamps and build logs were reviewed; the only
later application-code changes found were those four files. README and build
documentation were subsequently updated. `scripts/test-msix-packaging.ps1` now
uses its own placeholder identity fixture rather than assuming the real
publisher identity file contains placeholders. This test-only correction does
not change the application or package builder.

The retained **unsigned submission** package has these SHA-256 hashes:

| Artifact | SHA-256 |
| --- | --- |
| `NASAQ_0.3.10.0_x64.msix` | `10e316200b500d1aeb9cdfd840524fe85b744c97884bcaba7dd1184e9281b090` |
| `NASAQ.exe` inside that package | `45ceef28b15242e88730777f6d5f936ea6ff8fa2ab9c796969a0f258485e7f8d` |

The archive and executable hashes were checked again during source publication.
These identify the locally retained submission, **not** a newly downloaded
Store-signed package. Microsoft Store availability was observed on the public
product page; the publisher reported certification acceptance. This is not a
bit-for-bit reproducible-build claim, and the public Store listing alone does
not attest the currently delivered package version or source commit.

## Public activation configuration

The original release used these public CMake cache values. They are publisher
identifiers and a verification public key, not an API token or private key:

```text
NASAQ_REQUIRE_ACTIVATION_CONFIG=ON
NASAQ_KEYGEN_ACCOUNT=4fd35967-a371-4bf8-b9eb-8abf168330e0
NASAQ_KEYGEN_PRODUCT=2c81da3e-5c6a-4253-993f-c0924d4055ea
NASAQ_KEYGEN_PUBLIC_KEY=387a35987fb319f6babbf10e4ae1d8a9f74e3e6b7e51f7d2c449e916b9deb26e
```

Pass these as `-DNAME=value` when configuring the release. A third-party fork
should use its own publisher identity. No private signing key or admin API
token belongs in a client build.

The retained build cache uses `NASAQ_STORE_BUILD=OFF`; Store package identity
also disables external updates at runtime. New Store-specific builds can use
`-StoreBuild` / `NASAQ_STORE_BUILD=ON` as documented in [MSIX.md](MSIX.md).
See [BUILD.md](BUILD.md) for the MSVC, Qt and dependency versions.

## Validation record

[store-regression-results.json](store-regression-results.json) is the retained
October 17 run: seven test suites in Arabic and English, 236 passed checks,
zero failures and six skips. These are historical local results, not a fresh
full rebuild of this checkout, not GitHub CI, and not exhaustive hardware or
installed-Store-package acceptance. Publication additionally checks packaging
contracts, source content, release identity and the retained package hashes.

The application source remains GPL-3.0-or-later. Original notices, the upstream
source layout, dependencies documentation and [COPYING](../COPYING) are retained.
