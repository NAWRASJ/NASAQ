# SignPath readiness — 2026-10-14

Completed: NASAQ is a public GitHub fork of strawberrymusicplayer/strawberry.
Upstream ancestry, GPL notices, modified source, logo and project docs are retained.
The visible fork requirement is addressed; this is not a decision by SignPath.

Still required:

1. Confirm that the upstream release branches used for signing have signed releases.
   No verified upstream Windows signing evidence has been collected yet.
2. Resolve third-party signing coverage, especially libebur128 and Qt/GStreamer
   dependencies. The previous payload had 109 distinct unsigned PE hashes.
   Signing the installer alone does not establish trust for every loaded DLL.
3. Establish a source-verifiable Windows build pipeline, review controls, final
   approval roles and MFA, then obtain Foundation acceptance. Local builds do not
   demonstrate a successful GitHub Actions signing integration.
4. Complete dependency source/license inventory and privacy review before a release.
   Public source publication is not public binary distribution clearance.
5. Publish the appropriate reviewed release and evidence when ready, and submit the
   application through the Foundation's process. No application has been submitted.

References: https://signpath.org/terms.html and https://signpath.org/apply.html .
A public fork improves traceability; free signing and eligibility are not guaranteed.
