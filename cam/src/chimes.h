// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <functional>
class QWidget;
class Chimes {
 public:
 static QJsonArray library(Store &s);
 static QString resolve(Store &s,const QString &id);
 static QString importFile(Store &s,const QString &file,const QString &name,const QString &source,const QString &libraryDirectory = {});
 static int gap(Store &s);
};
void showChimes(QWidget *parent,Store &s);
// One cancellable cue; cancellation also invalidates its delayed continuation.
class ChimeCue:public QObject {
 public:
 explicit ChimeCue(QObject *parent=nullptr);
 void start(const QString &file,float volume,int gapMs,std::function<void()> done);
 void cancel();
 bool active()const{return bool(continuation);}
 std::function<void(const QString&)> error;
 private:
 void complete(bool failed=false);
 QMediaPlayer player;QAudioOutput output;QTimer delay,watchdog;
 std::function<void()> continuation;bool playingCue=false;int gapMs=0;
};
