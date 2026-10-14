// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDateTime>
#include <QJsonObject>
struct LicenseConfig {
 QString account, product, publicKey;
 int graceSeconds=7*86400;
 bool configured() const;
 static LicenseConfig production();
};
struct LicenseLease {
 bool valid=false;
 QString message, licenseId, machineId, key;
 QDateTime until;
 int maxMachines=0;
};
QString licenseRelation(const QJsonObject &object,const QString &name);
QString normalizeLicenseKey(QString key);
QString licenseFingerprint();
QByteArray protectLicenseData(const QByteArray &plain);
QByteArray unprotectLicenseData(const QByteArray &sealed);
LicenseLease verifyLicenseLease(const QByteArray &certificate,const LicenseConfig &config,
 const QString &fingerprint,const QDateTime &now,const QDateTime &lastSeen={});
