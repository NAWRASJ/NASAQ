// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
// Online validation adapter. No private vendor token; licence keys are transient.
class Activation:public QObject {
 Q_OBJECT
 public: explicit Activation(QObject*p=nullptr):QObject(p),network(this){}
 void validate(const QString &account,const QString &product,const QString &key);
 Q_SIGNALS:void result(bool valid,const QString &message);
 private:QNetworkAccessManager network;
};
