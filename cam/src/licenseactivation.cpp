// SPDX-License-Identifier: GPL-3.0-or-later
#include "licenseactivation.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
void Activation::validate(const QString&account,const QString&product,const QString&key){
 if(!QRegularExpression("^[a-zA-Z0-9-]+$").match(account).hasMatch()||product.isEmpty()||key.isEmpty()){emit result(false,"Account, product and licence key are required");return;}
 QNetworkRequest request(QUrl("https://api.keygen.sh/v1/accounts/"+account+"/licenses/actions/validate-key"));
 request.setHeader(QNetworkRequest::ContentTypeHeader,"application/vnd.api+json");request.setRawHeader("Accept","application/vnd.api+json");request.setTransferTimeout(10000);
 QJsonObject meta{{"key",key},{"scope",QJsonObject{{"product",product}}}};
 auto reply=network.post(request,QJsonDocument(QJsonObject{{"meta",meta}}).toJson());
 connect(reply,&QNetworkReply::finished,this,[this,reply,product]{
  bool valid=false;QString message="Validation failed or network unavailable";
  if(reply->error()==QNetworkReply::NoError){auto root=QJsonDocument::fromJson(reply->readAll()).object();auto data=root["data"].toObject();auto actualProduct=data["relationships"].toObject()["product"].toObject()["data"].toObject()["id"].toString();valid=root["meta"].toObject()["valid"].toBool(false)&&actualProduct==product;message=valid?"Online licence validated for this session":"Licence not valid for this product";}
  emit result(valid,message);reply->deleteLater();
 });
}
