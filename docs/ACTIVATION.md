# NASAQ activation — 0.3.0 engineering implementation

This branch adds activation to the official build. The publisher service is NOT configured: no live Keygen account, product, public key, or customer licences exist in this delivery. Missing configuration denies playback. There is no trial bypass or embedded production secret. Previous 0.2.1 binaries are unchanged and still behave as previews.

## Behaviour

- A publisher generates 16 random characters from A–Z and 2–9, excluding I/O. Clients may type XXXX-XXXX-XXXX-XXXX. The dashes do not count toward the 16 characters.
- The issuer specifies days from issuance and maximum registered devices per key. Renewal starts a new term from the time the command is run. Device limits are enforced by Keygen, with strict scope checks and NO_OVERAGE.
- Initial activation needs HTTPS access to api.keygen.sh. The client validates the key for this product and device, registers the device if permitted, revalidates, then requests a signed machine certificate.
- OpenSSL verifies Ed25519 signatures. Account, product, device, licence ID, active status and dates are checked. Cached allowance ends at the earliest of the certificate expiry, issuance + 7 days, and licence expiry.
- Startup restores an existing valid certificate; the app attempts online refresh at startup and hourly. If a restart occurs after certificate expiry, re-enter the key to reconnect. No audio automatically starts after activation/restart.
- Timeouts, 429 and server failures preserve only an already valid signed allowance. Explicit denial invalidates the cache. Revocation cannot reach an offline device immediately: its existing allowance can last up to 7 days or licence expiry, whichever is sooner.
- Windows DPAPI protects the saved certificate (which includes the customer's key) and last-seen time for the Windows user. Raw machine identity is not transmitted; a product-specific SHA-256 fingerprint is used. Keygen also sees the connection IP. No publisher administration token is shipped.
- Missing, invalid or expired activation stops music, announcements, adhan/TTS and paging, blocks Strawberry manual/resume playback and rejects scheduled occurrences that become due. Those occurrences are not replayed when reactivated. Schedules and logs remain; CSV/PDF, licence records, activation and STOP remain accessible subject to the existing operator PIN.
- An OS reinstall can change the device identity. A different Windows account must enter the same key again and receives its own protected cache. Release an obsolete device from the publisher dashboard or administrator tool.

## Publisher setup

1. Create a Keygen account and product. Choose a plan/deployment supporting the required API. No subscription has been purchased or created here.
2. On the publisher's computer, set KEYGEN_ACCOUNT_ID (UUID), KEYGEN_PRODUCT_ID and a suitably scoped KEYGEN_ADMIN_TOKEN in the process environment. Keep tokens in your password manager/secret store; do not paste them into chat, commit them, or put them in customer installers.
3. Run the publisher tool (Python 3 standard library):
   `python tools/licensing/nasaq_licenses.py create-policy`
   This previews the request. `python tools/licensing/nasaq_licenses.py --apply create-policy` creates the policy. Record its ID as KEYGEN_POLICY_ID.
4. Issue a 365-day, two-device key:
   `python tools/licensing/nasaq_licenses.py --apply issue --days 365 --devices 2`
   The real key is printed only after successful issuance. Deliver it privately to the customer; do not redirect it into public build logs. Without --apply the tool does not contact the service or issue a real key. The tool verifies the policy before issuance. No writes are retried automatically; check the dashboard after an ambiguous network failure.
5. Obtain the account's Ed25519 public key. Reconfigure the build with public values only:
   `-DNASAQ_KEYGEN_ACCOUNT=<account UUID> -DNASAQ_KEYGEN_PRODUCT=<product UUID> -DNASAQ_KEYGEN_PUBLIC_KEY=<64 hex characters>`
   Rebuild, test, package and sign that build. These public values are compiled into the application, not editable by customers in a settings dialog. The current unconfigured binary cannot be provisioned by typing account details.
6. Test against the real service: first device, repeated activation, device N+1, server outage, renewal, suspension, release/reassignment, expiry and restart. The current automated tests simulate provider responses; they do not demonstrate live API acceptance or concurrency enforcement.

Other tool operations (all take --apply before the subcommand):
- `renew --id <licence-id> --days 365`
- `set-devices --id <licence-id> --devices 3`
- `suspend --id <licence-id>` / `reinstate --id <licence-id>`
- `list-devices --id <licence-id>` (shows one provider page plus nextPage)
- `release-device --id <machine-id>` (delete an obsolete activation; an offline cached certificate remains valid until its deadline)

The issuer is a command-line administration tool, not yet a publisher web dashboard. Use Keygen's dashboard for account administration and review. Do not share the issuer token with customers.

## GPL and security boundaries

Strawberry and NASAQ source remain GPL-3.0-or-later. COPYING and upstream credits are retained. Activation is a mechanism in the official distribution and its associated service, not an additional legal restriction on GPL permissions. Users retain the rights to run, copy, modify, rebuild and redistribute GPL code; they can modify/remove the check. No EULA or promise of unremovable DRM is added. Fees may cover distribution, activation service, support and updates; they do not revoke GPL rights. Distribute corresponding source and build instructions alongside every binary; meet any applicable GPLv3 installation-information obligations.

Local administrators can restore old user profiles, clone device identity, change clocks or modify the program. DPAPI and the saved time detect ordinary tampering/rollback; they do not provide a trusted hardware clock or protection from the machine's administrator. The client has no private signing key. Test signing keys are generated at runtime and are never accepted by the production configuration.

Code signing for Windows (SignPath/Authenticode) is separate from customer activation. This implementation does not obtain a signing certificate, guarantee Smart App Control acceptance, or establish SignPath eligibility. The Foundation must evaluate the final distribution and service arrangement.

Sources checked 2026-10-14:
- https://keygen.sh/docs/api/licenses/
- https://keygen.sh/docs/api/policies/
- https://keygen.sh/docs/api/machines/
- https://keygen.sh/docs/api/cryptography/
- https://github.com/keygen-sh/example-python-cryptographic-machine-files (protocol reference; verifier uses OpenSSL)
- https://www.gnu.org/licenses/gpl-faq.en.html#DoesTheGPLAllowRequireFee
- https://www.gnu.org/licenses/gpl.en.html
