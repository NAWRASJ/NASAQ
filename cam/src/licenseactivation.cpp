// SPDX-License-Identifier: GPL-3.0-or-later
#include "licenseactivation.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
Activation::Activation(Store&s,QObject*p,LicenseConfig c,QNetworkAccessManager*n):QObject(p),store(s),config(c),network(n?n:new QNetworkAccessManager(this)),fingerprint(licenseFingerprint()){
 auto cached=QJsonDocument::fromJson(unprotectLicenseData(QByteArray::fromBase64(store.get("activation_sealed_v1").toLatin1()))).object();
 certificate=cached["certificate"].toString().toUtf8();lastSeen=QDateTime::fromString(cached["lastSeen"].toString(),Qt::ISODate);
 lease=verifyLicenseLease(certificate,config,fingerprint,QDateTime::currentDateTimeUtc(),lastSeen);key=lease.key;nextRefresh=QDateTime::currentDateTimeUtc();
}
void Activation::publish(){Q_EMIT changed(lease.valid,lease.message);}
void Activation::persist(){
 auto sealed=protectLicenseData(QJsonDocument(QJsonObject{{"certificate",QString::fromUtf8(certificate)},{"lastSeen",lastSeen.toString(Qt::ISODate)}}).toJson(QJsonDocument::Compact));
 if(sealed.isEmpty())throw std::runtime_error("Windows could not protect activation data");
 store.set("activation_sealed_v1",sealed.toBase64());lastSaved=QDateTime::currentDateTimeUtc();
}
void Activation::tick(){
 const auto now=QDateTime::currentDateTimeUtc();const bool before=lease.valid;lease=verifyLicenseLease(certificate,config,fingerprint,now,lastSeen);
 if(lease.valid){lastSeen=qMax(now,lastSeen);if(!lastSaved.isValid()||lastSaved.secsTo(now)>=60)persist();}
 if(before!=lease.valid)publish();if(!busy&&!key.isEmpty()&&now>=nextRefresh)activate(key);
}
void Activation::activate(const QString&input){
 if(busy)return;
 if(!config.configured()||fingerprint.isEmpty()){lease.valid=false;lease.message="خدمة التفعيل غير مهيأة لدى الناشر / Publisher setup required";publish();return;}
 auto normalized=normalizeLicenseKey(input);if(normalized.isEmpty()){Q_EMIT changed(lease.valid,"أدخل مفتاحًا من 16 حرفًا ورقمًا / Enter a 16-character key");return;}
 key=normalized;busy=true;validate(true);
}
void Activation::request(const QString&path,const QJsonObject&body,const Reply&callback){
 QNetworkRequest req(QUrl("https://api.keygen.sh/v1/accounts/"+config.account+path));
 req.setHeader(QNetworkRequest::ContentTypeHeader,"application/vnd.api+json");req.setRawHeader("Accept","application/vnd.api+json");req.setRawHeader("Authorization","License "+key.toLatin1());
 req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);req.setTransferTimeout(10000);
 auto reply=network->post(req,QJsonDocument(body).toJson(QJsonDocument::Compact));
 connect(reply,&QNetworkReply::finished,this,[this,reply,callback]{
  const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();const auto bytes=reply->readAll();const auto json=bytes.size()<=1024*1024?QJsonDocument::fromJson(bytes).object():QJsonObject();reply->deleteLater();
  try{callback(status,json);}catch(const std::exception&){busy=false;certificate.clear();key.clear();lease.valid=false;lease.message="Activation storage failed; reconnect after fixing storage";publish();}
 });
}
void Activation::finish(const QString&message,bool revoke){
 busy=false;nextRefresh=QDateTime::currentDateTimeUtc().addSecs(3600);
 if(revoke){certificate.clear();key.clear();persist();}
 lease=verifyLicenseLease(certificate,config,fingerprint,QDateTime::currentDateTimeUtc(),lastSeen);lease.message=message+(lease.valid?" · "+lease.message:QString());publish();
}
void Activation::validate(bool mayActivate){
 request("/licenses/actions/validate-key",{{"meta",QJsonObject{{"key",key},{"scope",QJsonObject{{"product",config.product},{"fingerprint",fingerprint}}}}}},[this,mayActivate](int status,const QJsonObject&root){
  if(status==0||status>=500||status==429){finish("تعذّر الاتصال؛ يُستخدم السماح المحفوظ إن كان صالحًا / Service unavailable");return;}
  if(status!=200){finish("رفض خدمة التفعيل / Activation request rejected",true);return;}
  auto data=root["data"].toObject(),meta=root["meta"].toObject();licenseId=data["id"].toString();
  if(licenseId.isEmpty()||licenseRelation(data,"product")!=config.product||licenseRelation(data,"account")!=config.account){finish("Licence does not belong to this publisher/product",true);return;}
  if(meta["valid"].toBool(false)){checkout();return;}
  const auto code=meta["code"].toString();
  if(mayActivate&&(code=="NO_MACHINE"||code=="NO_MACHINES"||code=="FINGERPRINT_SCOPE_MISMATCH")){
   QJsonObject link{{"data",QJsonObject{{"type","licenses"},{"id",licenseId}}}};
   QJsonObject machine{{"type","machines"},{"attributes",QJsonObject{{"fingerprint",fingerprint},{"platform","Windows"},{"name","NASAQ device"}}},{"relationships",QJsonObject{{"license",link}}}};
   request("/machines",{{"data",machine}},[this](int st,const QJsonObject&){if(st==201||st==409||st==422)validate(false);else if(st==0||st>=500||st==429)finish("Device activation temporarily unavailable");else finish("Device activation denied",true);});return;
  }
  finish("الترخيص غير صالح أو بلغ حد الأجهزة / Licence validation denied: "+code,true);
 });
}
void Activation::checkout(){
 request("/machines/"+fingerprint+"/actions/check-out?include=license&algorithm=base64%2Bed25519&ttl="+QString::number(config.graceSeconds),{},[this](int status,const QJsonObject&root){
  if(status==0||status>=500||status==429){finish("Offline certificate renewal unavailable");return;}
  if(status!=200){finish("Offline certificate rejected",true);return;}
  auto candidate=root["data"].toObject()["attributes"].toObject()["certificate"].toString().toUtf8();auto checked=verifyLicenseLease(candidate,config,fingerprint,QDateTime::currentDateTimeUtc(),lastSeen);
  if(!checked.valid||checked.licenseId!=licenseId||checked.key!=key){finish("Invalid signed activation certificate",true);return;}
  certificate=candidate;lease=checked;lastSeen=QDateTime::currentDateTimeUtc();persist();busy=false;nextRefresh=lastSeen.addSecs(3600);publish();
 });
}
