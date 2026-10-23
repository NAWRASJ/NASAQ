// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QJsonObject>
class CloudSpeech : public QObject {
 Q_OBJECT
public:
 explicit CloudSpeech(const QString &directory, QObject *parent=nullptr, QNetworkAccessManager *network=nullptr);
 ~CloudSpeech() override;
 static QJsonObject requestBody(const QString &text,const QString &language,const QString &voice);
 static QByteArray decodeAudio(const QByteArray &json);
 static bool validWave(const QByteArray &data);
 void generate(const QString &text,const QString &language,const QString &voice,const QString &project,const QString &token);
 void cancel();
 bool busy() const { return !reply.isNull(); }
 Q_SIGNALS:
 void ready(const QString &path,bool cached);
 void failed(const QString &code);
private:
 QString directory;
 QNetworkAccessManager *network;
 QPointer<QNetworkReply> reply;
};
