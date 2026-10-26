// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

inline bool NasaqLocalVideo(const QUrl &url) {
  static const QStringList extensions{QStringLiteral("mp4"),QStringLiteral("m4v"),QStringLiteral("mov"),QStringLiteral("mkv"),QStringLiteral("webm"),QStringLiteral("avi")};
  return url.isLocalFile() && extensions.contains(QFileInfo(url.toLocalFile()).suffix().toLower());
}
