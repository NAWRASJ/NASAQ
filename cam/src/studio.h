// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "audio.h"
#include <QJsonArray>
class Studio {
 public:
 Studio(Store &store,const QString &directory);
 QString importAudio(const QString &file);
 static qint64 duration(const QString &file);
 void exportAudio(const QString &file,const QString &destination);
 QString save(QJsonObject ad);
 QJsonObject load(const QString &id);
 QList<QJsonObject> library(const QString &search={});
 void remove(const QString &id);
 QString schedule(const QString &ad,const QJsonObject &rule,const QDateTime &now);
 static QDateTime nextSlot(const QJsonObject &rule,const QDateTime &from,bool inclusive=true);
 static QString payload(const QJsonObject &ad,const QStringList &languages,int gap);
 Store &db;QString root;
};
class StudioEngine:public QObject {
 Q_OBJECT
 public: StudioEngine(Studio &studio,Audio &audio,QObject *parent=nullptr);
 void tick(const QDateTime &now);
 private: Studio &studio;Audio &audio;
};
