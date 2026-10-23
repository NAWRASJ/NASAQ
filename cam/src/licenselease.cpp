// SPDX-License-Identifier: GPL-3.0-or-later
#include "licenselease.h"
#include "licenseconfig.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QSysInfo>
#include <openssl/evp.h>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif
bool LicenseConfig::configured()const{
 const QRegularExpression id("^[A-Za-z0-9-]+$"),hex("^[A-Fa-f0-9]{64}$");
 return id.match(account).hasMatch()&&id.match(product).hasMatch()&&hex.match(publicKey).hasMatch()&&graceSeconds>=3600&&graceSeconds<=7*86400;
}
LicenseConfig LicenseConfig::production(){return {NASAQ_KEYGEN_ACCOUNT,NASAQ_KEYGEN_PRODUCT,NASAQ_KEYGEN_PUBLIC_KEY};}
QString licenseRelation(const QJsonObject&o,const QString&n){return o["relationships"].toObject()[n].toObject()["data"].toObject()["id"].toString();}
QString normalizeLicenseKey(QString k){k.remove('-');k.remove(' ');k=k.toUpper();return QRegularExpression("^[A-HJ-NP-Z2-9]{16}$").match(k).hasMatch()?k:QString();}
QString licenseFingerprint(){const auto id=QSysInfo::machineUniqueId();if(id.isEmpty())return {};return QCryptographicHash::hash("NASAQ/device/v1/"+id,QCryptographicHash::Sha256).toHex();}
QByteArray protectLicenseData(const QByteArray &plain){
#ifdef Q_OS_WIN
 DATA_BLOB in{DWORD(plain.size()),reinterpret_cast<BYTE*>(const_cast<char*>(plain.constData()))},out{};
 if(!CryptProtectData(&in,L"NASAQ activation",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))return {};
 QByteArray result(reinterpret_cast<char*>(out.pbData),out.cbData);LocalFree(out.pbData);return result;
#else
 Q_UNUSED(plain);return {}; // No insecure plaintext fallback.
#endif
}
QByteArray unprotectLicenseData(const QByteArray &sealed){
#ifdef Q_OS_WIN
 DATA_BLOB in{DWORD(sealed.size()),reinterpret_cast<BYTE*>(const_cast<char*>(sealed.constData()))},out{};
 if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))return {};
 QByteArray result(reinterpret_cast<char*>(out.pbData),out.cbData);LocalFree(out.pbData);return result;
#else
 Q_UNUSED(sealed);return {};
#endif
}
LicenseLease verifyLicenseLease(const QByteArray&cert,const LicenseConfig&c,const QString&fingerprint,const QDateTime&now,const QDateTime&lastSeen){
 LicenseLease r;auto fail=[&](const QString&m){r.valid=false;r.key.clear();r.message=m;return r;};
 if(!c.configured()||fingerprint.isEmpty())return fail("Publisher activation service is not configured / خدمة التفعيل غير مهيأة");
 if(!now.isValid()||(lastSeen.isValid()&&now.addSecs(300)<lastSeen))return fail("Clock rollback detected; restore the correct time / تحقق من ساعة الجهاز");
 QByteArray body=cert.trimmed();const QByteArray begin="-----BEGIN MACHINE FILE-----",end="-----END MACHINE FILE-----";
 if(body.size()>1024*1024||!body.startsWith(begin)||!body.endsWith(end))return fail("Activation required / يلزم التفعيل");
 body=body.mid(begin.size(),body.size()-begin.size()-end.size()).simplified();body.replace(" ","");
 auto envelope=QJsonDocument::fromJson(QByteArray::fromBase64(body)).object();
 if(envelope["alg"].toString()!="base64+ed25519")return fail("Unsupported licence signature");
 const auto enc=envelope["enc"].toString().toLatin1(),sig=QByteArray::fromBase64(envelope["sig"].toString().toLatin1()),pub=QByteArray::fromHex(c.publicKey.toLatin1()),message="machine/"+enc;
 EVP_PKEY *key=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,reinterpret_cast<const unsigned char*>(pub.constData()),pub.size());
 EVP_MD_CTX *ctx=EVP_MD_CTX_new();
 const bool verified=key&&ctx&&sig.size()==64&&EVP_DigestVerifyInit(ctx,nullptr,nullptr,nullptr,key)==1&&EVP_DigestVerify(ctx,reinterpret_cast<const unsigned char*>(sig.constData()),sig.size(),reinterpret_cast<const unsigned char*>(message.constData()),message.size())==1;
 EVP_MD_CTX_free(ctx);EVP_PKEY_free(key);if(!verified)return fail("Invalid licence signature / توقيع الترخيص غير صالح");
 auto doc=QJsonDocument::fromJson(QByteArray::fromBase64(enc)).object(),machine=doc["data"].toObject(),meta=doc["meta"].toObject();
 if(machine["type"].toString()!="machines"||machine["id"].toString().isEmpty()||licenseRelation(machine,"account")!=c.account||licenseRelation(machine,"product")!=c.product||machine["attributes"].toObject()["fingerprint"].toString()!=fingerprint)return fail("Licence belongs to another device or product");
 QJsonObject license;for(const auto &value:doc["included"].toArray()){auto o=value.toObject();if(o["type"].toString()=="licenses"&&o["id"].toString()==licenseRelation(machine,"license"))license=o;}
 auto a=license["attributes"].toObject();
 if(license.isEmpty()||licenseRelation(license,"account")!=c.account||licenseRelation(license,"product")!=c.product||a["suspended"].toBool(true)||a["status"].toString()!="ACTIVE"||a["maxMachines"].toInt()<1)return fail("Licence is not active");
 auto issued=QDateTime::fromString(meta["issued"].toString(),Qt::ISODate),expiry=QDateTime::fromString(meta["expiry"].toString(),Qt::ISODate),licenseExpiry=QDateTime::fromString(a["expiry"].toString(),Qt::ISODate);
 if(!issued.isValid()||!expiry.isValid()||!licenseExpiry.isValid()||issued>now.addSecs(300)||expiry<=issued||meta["ttl"].toDouble()<=0)return fail("Invalid licence dates");
 r.expiresAt=licenseExpiry;
 r.until=qMin(qMin(expiry,issued.addSecs(c.graceSeconds)),licenseExpiry);
 if(now>=r.until)return fail("Licence or offline grace expired; reconnect / انتهى الترخيص أو السماح دون إنترنت");
 r.key=normalizeLicenseKey(a["key"].toString());if(r.key.isEmpty())return fail("Invalid licence key format");
 r.valid=true;r.licenseId=license["id"].toString();r.machineId=machine["id"].toString();r.maxMachines=a["maxMachines"].toInt();
 r.message=QString("مفعّل / Active · devices: %1 · offline until: %2").arg(r.maxMachines).arg(r.until.toLocalTime().toString("yyyy-MM-dd HH:mm"));return r;
}
