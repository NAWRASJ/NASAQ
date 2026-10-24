# In-app Microsoft Store updates

Settings includes Check for updates and Update via Microsoft Store. Packaged
builds check 30 seconds after the workspace is constructed and every six hours
while it remains open. A positive result shows a banner across workspace pages.
Opening the Store is explicit; the application does not install or restart itself.

The worker uses StoreContext.GetAppAndOptionalStorePackageUpdatesAsync with the
desktop owner window initialized through IInitializeWithWindow. Checks time out
after 20 seconds. Errors are distinct from a successful check with no updates;
an error does not withdraw a previously displayed update notification.

Unpackaged builds can open the Store but do not claim to know its update status.
The existing manual GitHub release lookup remains available outside Store builds.

Local tests simulate available, unavailable and failed queries, concurrent click
suppression, and unpackaged behavior. Actual Store eligibility requires testing
the installed Store app under its retail license, including an available newer
package. This cannot be established by a local source build or GitHub release.

Before distribution, verify on an installed Store build: manual check, offline
failure, background positive notification, and opening the product page for
9PH1T4418KTF. Confirm music is not interrupted by checking or opening the Store.

Reference: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/package-updates-from-store
