// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
class QWidget;
namespace NasaqData {
QString profilePath();
void backup(const QString &root, const QString &destination);
void validateBackup(const QString &path);
void queueRestore(const QString &root,const QString &path,bool keepLicense=true);
void queueReset(const QString &root,bool keepLicense=true);
bool applyPending(const QString &root);
void showDialog(QWidget *parent);
}
