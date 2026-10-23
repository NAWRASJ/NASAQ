// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#include <appmodel.h>
#endif
namespace Nasaq {
inline bool isPackaged() {
#ifdef Q_OS_WIN
  UINT32 length = 0;
  const LONG result = GetCurrentPackageFullName(&length, nullptr);
  // Only an actual package identity enables packaged behaviour. Unexpected
  // API errors must not masquerade as a successful identity lookup.
  return result == ERROR_INSUFFICIENT_BUFFER || result == ERROR_SUCCESS;
#else
  return false;
#endif
}
inline bool storeManaged() {
#ifdef NASAQ_STORE_BUILD
  return true;
#else
  return isPackaged();
#endif
}
}
