// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTextToSpeech>
#include <QAudioSource>
#include <QAudioSink>
#include <memory>
#include "musictransport.h"
class Audio:public QObject {
 Q_OBJECT
 public:
 explicit Audio(Store&s,QObject*parent=nullptr,MusicTransport*transport=nullptr);
 bool playMusic(const QStringList&files);
 bool dispatch(const Event&e);
 bool page(bool on);
 void stop();
 int activePriority()const{return priority;}
 void setVoice(const QVoice&v){speech.setLocale(v.locale());speech.setVoice(v);}
 QList<QVoice> voices(){QList<QVoice> result;if(priority!=0)return result;auto previous=speech.locale();for(const auto&locale:speech.availableLocales()){speech.setLocale(locale);result.append(speech.availableVoices());}speech.setLocale(previous);return result;}
 Q_SIGNALS: void status(const QString&text);
 private:
 void release(); void log(const QString&,const QString&);
 Store &store; MusicTransport *music; QMediaPlayer overlay; QAudioOutput overlayOut; QTextToSpeech speech;
 int priority=0; bool resume=false,paging=false,transition=false;
 std::unique_ptr<QAudioSource> mic; std::unique_ptr<QAudioSink> sink;
};
