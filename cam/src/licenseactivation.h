// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include "licenselease.h"
#include "store.h"
class Activation:public QObject {
 Q_OBJECT
 public:
 explicit Activation(Store &store,QObject *parent=nullptr,LicenseConfig config=LicenseConfig::production(),QNetworkAccessManager *transport=nullptr);
 bool allowed()const{return lease.valid;}
 QString description()const{return lease.message;}
 LicenseLease details()const{return lease;}
 void activate(const QString &key);
 void tick();
 Q_SIGNALS: void changed(bool allowed,const QString &message);
 void completed(bool success);
 private:
 using Reply=std::function<void(int,const QJsonObject&)>;
 void request(const QString &path,const QJsonObject &body,const Reply &callback);
 void validate(bool mayActivate);void checkout();void finish(const QString &message,bool revoke=false);void persist();void publish();
 Store &store;LicenseConfig config;QNetworkAccessManager *network;
 LicenseLease lease;QString fingerprint,key,licenseId;QByteArray certificate;
 QDateTime lastSeen,lastSaved,nextRefresh;bool busy=false;
};
