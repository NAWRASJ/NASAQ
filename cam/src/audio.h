// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include "musicduck.h"
#include "chimes.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTextToSpeech>
#include <QAudioSource>
#include <QAudioSink>
#include <memory>
#include <QTimer>
#include "musictransport.h"
class Audio:public QObject {
 Q_OBJECT
 public:
 explicit Audio(Store&s,QObject*parent=nullptr,MusicTransport*transport=nullptr);
 bool playMusic(const QStringList&files);
 bool dispatch(const Event&e);
 bool page(bool on);
 void tick(const QDateTime &now);
 bool prayerActive()const{return prayerUntil.isValid();}
 bool skipPrayer();
 void setPrayerPauseEnabled(bool enabled);
 void stop();
 void cancelSequence(const QString &id){if(sequenceId==id)stop();}
 void setLicenseAllowed(bool allowed,bool stopPlayback=true){licenseAllowed=allowed;if(!allowed&&stopPlayback)stop();}
 int activePriority()const{return priority;}
 void setVoice(const QVoice&v){speech.setLocale(v.locale());speech.setVoice(v);store.set(QStringLiteral("tts_voice"),v.name());store.set(QStringLiteral("tts_locale"),v.locale().name());}
 QList<QVoice> voices(){QList<QVoice> result;if(priority!=0)return result;auto previous=speech.locale();auto previousVoice=speech.voice();for(const auto&locale:speech.availableLocales()){speech.setLocale(locale);result.append(speech.availableVoices());}speech.setLocale(previous);speech.setVoice(previousVoice);return result;}
 Q_SIGNALS: void sequenceFinished(const QString &id,const QString &result);
 void status(const QString&text);
 void pagingState(const QString &state);
 void prayerPauseChanged(const QDateTime &until);
 private:
 void finishSequence(const QString &result); void nextSequence();
 QString sequenceId;QStringList sequenceFiles,sequenceLanguages;int sequenceIndex=0,sequenceGap=0;QTimer sequenceTimer;
 void beforeChime(const QString &id,std::function<void()> done); bool openMicrophone();
 ChimeCue cue;bool pagePending=false;
 void release(); void log(const QString&,const QString&);
 Store &store; MusicTransport *music;MusicDuck duck;QDateTime prayerUntil;bool prayerRestored=false; QMediaPlayer overlay; QAudioOutput overlayOut; QTextToSpeech speech;
 bool licenseAllowed=true;
 int priority=0; bool resume=false,paging=false,transition=false;
 std::unique_ptr<QAudioSource> mic; std::unique_ptr<QAudioSink> sink;
};
