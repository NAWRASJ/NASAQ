// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

inline bool NasaqLocalVideo(const QUrl &url) {
  static const QStringList extensions{QStringLiteral("mp4"),QStringLiteral("m4v"),QStringLiteral("mov"),QStringLiteral("mkv"),QStringLiteral("webm"),QStringLiteral("avi"),QStringLiteral("wmv"),QStringLiteral("asf"),QStringLiteral("flv"),QStringLiteral("mpg"),QStringLiteral("mpeg"),QStringLiteral("mpe"),QStringLiteral("ts"),QStringLiteral("mts"),QStringLiteral("m2ts"),QStringLiteral("ogv"),QStringLiteral("3gp"),QStringLiteral("3g2")};
  return url.isLocalFile() && extensions.contains(QFileInfo(url.toLocalFile()).suffix().toLower());
}
