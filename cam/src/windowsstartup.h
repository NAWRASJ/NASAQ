// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
namespace Nasaq {
// Called on a worker thread: WinRT operations must not block the UI apartment.
QString setWindowsStartup(bool enabled);
bool microphonePrivacyDenied();
}
