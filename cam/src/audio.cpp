// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio.h"
#include <QMediaDevices>
#include <QFileInfo>
Audio::Audio(Store&s,QObject*p,MusicTransport*transport):QObject(p),store(s),music(transport?transport:new QtMusicTransport(this)),speech(QStringLiteral("sapi"),this){
 overlay.setAudioOutput(&overlayOut);overlayOut.setVolume(.7);
 connect(music,&MusicTransport::event,this,&Audio::log);
 connect(&overlay,&QMediaPlayer::mediaStatusChanged,this,[this](auto st){if(st==QMediaPlayer::EndOfMedia)release();});
 for(auto player:{&overlay}){
 connect(player,&QMediaPlayer::playbackStateChanged,this,[this,player](auto st){if(st==QMediaPlayer::PlayingState)log("play_started",player->source().toLocalFile());else if(st==QMediaPlayer::StoppedState)log("play_stopped",player->source().toLocalFile());});
 connect(player,&QMediaPlayer::errorOccurred,this,[this,player](auto,const QString&e){log("audio_error",e);if(player==&overlay&&(priority==50||priority==60))release();});}
 connect(&speech,&QTextToSpeech::stateChanged,this,[this](auto st){if(st==QTextToSpeech::Ready && priority==40)release();if(st==QTextToSpeech::Error){log("tts_error",speech.errorString());if(priority==40)release();}});
}
void Audio::log(const QString&k,const QString&d){try{store.log(k,d);}catch(const std::exception&e){Q_EMIT status(QStringLiteral("Log storage failed: ")+QString::fromUtf8(e.what()));return;}Q_EMIT status(k+": "+d);}
bool Audio::playMusic(const QStringList&files){if(!licenseAllowed){Q_EMIT status("Activation required / يلزم التفعيل");return false;}bool accepted=music->replace(files);if(accepted&&priority>0)resume=true;return accepted;}
void Audio::release(){if(transition||paging)return;priority=0;music->suspend(false);if(resume){resume=false;music->resume();}}
bool Audio::dispatch(const Event&e){
 if(!licenseAllowed)return false;
 if(e.kind=="prayer"){log("prayer_reminder",e.id);return true;}
 if(e.kind=="music")return playMusic(e.payload.split('\n',Qt::SkipEmptyParts));
 int requested=e.kind=="adhan"?60:e.kind=="tts"?40:50;
 if(priority>=requested)return false;
 if(e.kind=="tts"){if(speech.availableVoices().isEmpty()||e.payload.trimmed().isEmpty())return false;}
 else if((e.kind!="announcement"&&e.kind!="adhan")||!QFileInfo(e.payload).isFile())return false;
 resume=resume||music->playing();music->suspend(true);
 transition=true;overlay.stop();speech.stop();transition=false;priority=requested;
 if(e.kind=="tts")speech.say(e.payload);else{overlay.setSource(QUrl::fromLocalFile(e.payload));overlay.play();}
 return true;
}
bool Audio::page(bool on){
 if(!on){if(!paging)return true;mic->stop();sink->stop();mic.reset();sink.reset();paging=false;log("paging","ended");release();return true;}
 if(!licenseAllowed)return false;
 if(paging)return true;
 auto input=QMediaDevices::defaultAudioInput(),output=QMediaDevices::defaultAudioOutput();if(input.isNull()||output.isNull())return false;
 auto format=input.preferredFormat();if(!output.isFormatSupported(format))return false;
 resume=resume||music->playing();music->suspend(true);transition=true;overlay.stop();speech.stop();transition=false;priority=100;
 sink=std::make_unique<QAudioSink>(output,format);mic=std::make_unique<QAudioSource>(input,format);
 auto io=sink->start();if(!io){sink.reset();mic.reset();release();return false;}mic->start(io);
 if(mic->error()!=QAudio::NoError){mic->stop();sink->stop();mic.reset();sink.reset();release();return false;}
 paging=true;log("paging","started");return true;
}
void Audio::stop(){resume=false;transition=true;if(paging)page(false);priority=0;music->stop();overlay.stop();speech.stop();transition=false;music->suspend(false);log("operator","stop_all");}
