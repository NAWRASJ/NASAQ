// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QStringList>
#include <QMediaPlayer>
#include <QAudioOutput>
class MusicTransport:public QObject {
 Q_OBJECT
 public:
 using QObject::QObject;
 virtual bool replace(const QStringList&files)=0;
 virtual bool playing()const=0;
 virtual void suspend(bool active)=0;
 virtual void resume()=0;
 virtual void stop()=0;
 Q_SIGNALS:
 void event(const QString &kind,const QString &detail);
};
class QtMusicTransport:public MusicTransport {
 Q_OBJECT
 public:
 explicit QtMusicTransport(QObject*parent=nullptr);
 bool replace(const QStringList&files)override;
 bool playing()const override;
 void suspend(bool active)override;
 void resume()override;
 void stop()override;
 private:
 void next();QMediaPlayer player;QAudioOutput output;QStringList files;int index=-1;bool suspended=false;
};
