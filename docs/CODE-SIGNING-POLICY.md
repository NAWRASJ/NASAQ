# Code signing policy

Microsoft Store is an additional distribution target: see [MSIX](MSIX.md).
Submit the unsigned package with the exact public Partner Center identity;
Microsoft signs it after certification. Local-test MSIX certificates are not
production signing identities and must never be silently installed/trusted.
The Inno/Authenticode workflow below remains available for non-Store releases.

Status: preparation only; **not enrolled in or approved by SignPath Foundation**.
There is currently no production signing identity. No binaries in this project
should be described as SignPath-signed unless a verified release states otherwise.

Repository owner and release contact: @NAWRASJ. Changes and release inputs must be
reviewed before a signing request; approval remains a human maintainer action.
All maintainers must enable MFA before enrollment. MFA was not verified by this audit.
Contributors do not receive automatic access to signing credentials or approval.

Sign only reviewed, traceable builds from this repository. Validate source revisions,
dependency provenance, notices and artifact hashes. Preserve vendor signatures.
Do not sign third-party binaries under a Foundation subscription without an allowed
path confirmed by the Foundation/upstream maintainers. Do not weaken Windows policy.

The local certificate script is for a publisher-controlled trusted certificate;
it is not a configured SignPath pipeline. No signing tokens belong in source control.
The inherited upstream workflow is not a verified NASAQ Windows release pipeline.

If accepted, update this page with the Foundation-approved attribution, roles and
verified build/signing policy. Until then, do not display its sponsorship badge.
See [privacy](PRIVACY.md) and [remaining eligibility work](SIGNPATH-READINESS.md).
