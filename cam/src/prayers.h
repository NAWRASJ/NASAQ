// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include <QNetworkAccessManager>
class PrayerService:public QObject {
 Q_OBJECT
public:
 explicit PrayerService(Store &store,QObject *parent=nullptr);
 void sync(const QString &city,const QString &country,int method,const QString &adhan,bool enabled);
 static QList<Event> parseCalendar(const QByteArray &json,const QString &identity,const QString &audio,const QDateTime &now);
 static void saveCalendar(Store &store,const QList<Event> &events);
 static QDateTime nextPause(Store &store,const QDateTime &now);
Q_SIGNALS:
 void completed(bool ok,const QString &message);
private:
 Store &store;QNetworkAccessManager network;bool busy=false;
 void refreshSaved();
};
