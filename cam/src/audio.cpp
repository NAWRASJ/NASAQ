// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio.h"
#include "windowsstartup.h"
#include <QMediaDevices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
Audio::Audio(Store&s,QObject*p,MusicTransport*transport):QObject(p),store(s),music(transport?transport:new QtMusicTransport(this)),duck([this](double gain){music->setGain(gain);},this),speech(QStringLiteral("sapi"),this){
 const auto locale=store.get("tts_locale");if(!locale.isEmpty()){speech.setLocale(QLocale(locale));for(const auto &v:speech.availableVoices())if(v.name()==store.get("tts_voice")){speech.setVoice(v);break;}}
 sequenceTimer.setSingleShot(true);connect(&sequenceTimer,&QTimer::timeout,this,&Audio::nextSequence);
 overlay.setAudioOutput(&overlayOut);overlayOut.setVolume(.7);
 connect(music,&MusicTransport::event,this,&Audio::log);
 connect(&overlay,&QMediaPlayer::mediaStatusChanged,this,[this](auto st){if(st==QMediaPlayer::EndOfMedia){if(!sequenceId.isEmpty()){log("studio_language_completed",sequenceId+":"+sequenceLanguages.value(sequenceIndex));++sequenceIndex;if(sequenceIndex<sequenceFiles.size())sequenceTimer.start(sequenceGap*1000);else{finishSequence("completed");release();}}else release();}});
 for(auto player:{&overlay}){
 connect(player,&QMediaPlayer::playbackStateChanged,this,[this,player](auto st){if(st==QMediaPlayer::PlayingState)log("play_started",player->source().toLocalFile());else if(st==QMediaPlayer::StoppedState)log("play_stopped",player->source().toLocalFile());});
 connect(player,&QMediaPlayer::errorOccurred,this,[this,player](auto,const QString&e){log("audio_error",e);if(player==&overlay&&(priority==50||priority==60)){finishSequence("failed");release();}});}
 connect(&speech,&QTextToSpeech::stateChanged,this,[this](auto st){if(st==QTextToSpeech::Ready && priority==40)release();if(st==QTextToSpeech::Error){log("tts_error",speech.errorString());if(priority==40)release();}});
}
void Audio::log(const QString&k,const QString&d){try{store.log(k,d);}catch(const std::exception&e){Q_EMIT status(QStringLiteral("Log storage failed: ")+QString::fromUtf8(e.what()));return;}Q_EMIT status(k+": "+d);}
bool Audio::playMusic(const QStringList&files){if(!licenseAllowed){Q_EMIT status("Activation required / يلزم التفعيل");return false;}bool accepted=music->replace(files);if(accepted&&priority>0)resume=true;return accepted;}
void Audio::finishSequence(const QString &result){sequenceTimer.stop();if(sequenceId.isEmpty())return;auto id=sequenceId;sequenceId.clear();sequenceFiles.clear();sequenceLanguages.clear();log("studio_sequence_"+result,id);Q_EMIT sequenceFinished(id,result);}
void Audio::nextSequence(){if(sequenceId.isEmpty()||sequenceIndex>=sequenceFiles.size())return;log("studio_language_started",sequenceId+":"+sequenceLanguages.value(sequenceIndex));overlay.setSource(QUrl::fromLocalFile(sequenceFiles[sequenceIndex]));overlay.play();}
void Audio::release(){if(transition||paging||pagePending)return;if(prayerActive()){priority=60;music->suspend(true);return;}priority=0;music->suspend(false);if(resume){resume=false;music->resume();}}
bool Audio::dispatch(const Event&e){
 if(!licenseAllowed)return false;
 if((e.kind=="prayer"||e.kind=="adhan")&&store.get("prayer_pause_enabled")=="0"){log("adhan_disabled",e.id);return true;}
 if((e.kind=="prayer"||e.kind=="adhan")&&store.get("prayer_pause_enabled")!="0"){
 int minutes=store.get("prayer_pause_minutes").isEmpty()?10:qBound(1,store.get("prayer_pause_minutes").toInt(),60);
 auto now=QDateTime::currentDateTimeUtc();auto end=(e.due.isValid()?e.due:now).addSecs(minutes*60);if(end<=now)return false;
 prayerUntil=qMax(prayerUntil,end);store.set("prayer_hold_until",prayerUntil.toString(Qt::ISODate));resume=resume||music->playing();music->suspend(true);log("prayer_music_paused",e.id);Q_EMIT prayerPauseChanged(prayerUntil);
 if(!paging&&!pagePending){cue.cancel();transition=true;finishSequence("interrupted");overlay.stop();speech.stop();transition=false;priority=0;}
 }
 if(e.kind=="prayer"){log("prayer_reminder",e.id);if(prayerActive())release();return true;}
 if(e.kind=="music")return playMusic(e.payload.split('\n',Qt::SkipEmptyParts));
 if(prayerActive()&&e.kind!="adhan")return false;
 int requested=e.kind=="adhan"?60:e.kind=="tts"?40:50;
 if(priority>=requested)return false;
 QString speechText=e.payload;
 if(e.kind=="tts"){
  if(speech.errorReason()!=QTextToSpeech::ErrorReason::NoError){
   // Recreate the provider before retrying, so a stale error cannot be
   // mistaken for a failure of a later successful request. Restore the voice.
   const auto locale=speech.locale();const auto voice=speech.voice();
   speech.setEngine(QString());
   if(speech.engine()!=QStringLiteral("sapi"))speech.setEngine(QStringLiteral("sapi"));
   speech.setLocale(locale);speech.setVoice(voice);
  }
  const auto obj=QJsonDocument::fromJson(e.payload.toUtf8()).object();
  if(obj.contains("nasaqTts")){
   speechText=obj["text"].toString();speech.setLocale(QLocale(obj["locale"].toString()));bool found=false;
   for(const auto &v:speech.availableVoices())if(v.name()==obj["voice"].toString()){speech.setVoice(v);found=true;break;}
   if(!found){log("tts_error","Scheduled voice is not installed");return false;}
  }
  if(speech.availableVoices().isEmpty()||speechText.trimmed().isEmpty()||speech.state()==QTextToSpeech::Error)return false;
 }
 else if(e.kind=="sequence"){
 const auto obj=QJsonDocument::fromJson(e.payload.toUtf8()).object();auto files=obj["files"].toArray(),languages=obj["languages"].toArray();if(files.isEmpty()||files.size()>3||files.size()!=languages.size()||obj["gap"].toInt()<0||obj["gap"].toInt()>30)return false;for(auto file:files)if(!QFileInfo(file.toString()).isFile())return false;
 }
 else if((e.kind!="announcement"&&e.kind!="adhan")||!QFileInfo(e.payload).isFile()){if(prayerActive())release();return false;}
 resume=resume||music->playing();music->suspend(true);
 cue.cancel();transition=true;finishSequence("interrupted");overlay.stop();speech.stop();transition=false;priority=requested;
 if(e.kind=="sequence"){auto obj=QJsonDocument::fromJson(e.payload.toUtf8()).object();sequenceId=e.id;sequenceIndex=0;sequenceGap=obj["gap"].toInt();for(auto f:obj["files"].toArray())sequenceFiles<<f.toString();for(auto l:obj["languages"].toArray())sequenceLanguages<<l.toString();beforeChime(obj["chime"].toString("default"),[this]{nextSequence();});}
 else if(e.kind=="tts")beforeChime("default",[this,speechText]{
  speech.say(speechText);
  // SAPI can report a synchronous synthesis failure via errorReason() while
  // remaining Ready and emitting neither stateChanged nor errorOccurred.
  // Do not leave the announcement priority holding music indefinitely.
  if(priority==40 && speech.errorReason()!=QTextToSpeech::ErrorReason::NoError){
   log("tts_error",speech.errorString());release();
  }
 });else{auto play=[this,path=e.payload]{overlay.setSource(QUrl::fromLocalFile(path));overlay.play();};if(e.kind=="adhan"||e.id=="studio-preview")play();else beforeChime("default",play);}
 return true;
}
void Audio::beforeChime(const QString &selection,std::function<void()> done){
 auto id=selection=="default"?store.get("chime_ad"):selection;
 auto file=Chimes::resolve(store,id);if(file.isEmpty()){done();return;}
 log("chime_started",id);cue.error=[this](const QString &e){log("chime_failed",e);};
 auto volume=store.get("chime_volume").isEmpty()?70:store.get("chime_volume").toInt();
 cue.start(file,volume/100.f,Chimes::gap(store),std::move(done));
}
bool Audio::page(bool on){
 if(!on){
  if(pagePending){pagePending=false;cue.cancel();duck.release();Q_EMIT pagingState("idle");release();return true;}
  if(!paging)return true;paging=false;if(mic)mic->stop();if(sink)sink->stop();mic.reset();sink.reset();duck.release();Q_EMIT pagingState("idle");log("paging","ended");release();return true;
 }
 if(!licenseAllowed)return false;if(paging||pagePending)return true;
 if(Nasaq::microphonePrivacyDenied()){Q_EMIT pagingState("failed");return false;}
 auto input=QMediaDevices::defaultAudioInput(),output=QMediaDevices::defaultAudioOutput();
 if(input.isNull()||output.isNull()||!output.isFormatSupported(input.preferredFormat()))return false;
 cue.cancel();resume=resume||music->playing();transition=true;finishSequence("interrupted");overlay.stop();speech.stop();transition=false;priority=100;
 pagePending=true;duck.press();if(!prayerActive()){music->suspend(false);if(resume){resume=false;music->resume();}}else music->suspend(true);
 Q_EMIT pagingState("waiting");
 beforeChime(store.get("chime_mic"),[this]{if(pagePending&&!openMicrophone()){pagePending=false;duck.release();Q_EMIT pagingState("failed");release();}});return paging||pagePending;
}
bool Audio::openMicrophone(){
 if(!licenseAllowed||!pagePending)return false;
 auto input=QMediaDevices::defaultAudioInput(),output=QMediaDevices::defaultAudioOutput();auto format=input.preferredFormat();
 if(input.isNull()||output.isNull()||!output.isFormatSupported(format))return false;
 sink=std::make_unique<QAudioSink>(output,format);mic=std::make_unique<QAudioSource>(input,format);auto io=sink->start();
 if(!io){sink.reset();mic.reset();return false;}mic->start(io);
 if(mic->error()!=QAudio::NoError){mic->stop();sink->stop();mic.reset();sink.reset();return false;}
 pagePending=false;paging=true;Q_EMIT pagingState("live");
 connect(mic.get(),&QAudioSource::stateChanged,this,[this](QAudio::State state){if(paging&&state==QAudio::StoppedState&&mic&&mic->error()!=QAudio::NoError)QTimer::singleShot(0,this,[this]{page(false);Q_EMIT pagingState("failed");});});
 connect(sink.get(),&QAudioSink::stateChanged,this,[this](QAudio::State state){if(paging&&state==QAudio::StoppedState&&sink&&sink->error()!=QAudio::NoError)QTimer::singleShot(0,this,[this]{page(false);Q_EMIT pagingState("failed");});});log("paging","started");return true;
}
void Audio::stop(){cue.cancel();finishSequence("cancelled");resume=false;transition=true;if(paging||pagePending)page(false);priority=prayerActive()?60:0;music->stop();overlay.stop();speech.stop();transition=false;duck.reset();music->suspend(prayerActive());log("operator","stop_all");}

void Audio::setPrayerPauseEnabled(bool enabled){
 store.set("prayer_pause_enabled",enabled?"1":"0");
 if(!enabled){
  store.set("prayer_hold_until","");prayerUntil={};prayerRestored=true;
  if(priority==60){transition=true;overlay.stop();transition=false;if(!licenseAllowed)resume=false;release();}
  if(!licenseAllowed)music->stop();
  Q_EMIT prayerPauseChanged({});
 }
 log("adhan_auto_pause",enabled?"enabled":"disabled");
}
bool Audio::skipPrayer(){
 if(!licenseAllowed||!prayerActive())return false;
 store.set("prayer_hold_until","");prayerUntil={};prayerRestored=true;
 log("prayer_pause_skipped","operator requested playback; future prayers unchanged");
 if(!paging&&!pagePending){transition=true;overlay.stop();transition=false;resume=true;release();}
 Q_EMIT prayerPauseChanged({});return true;
}
void Audio::tick(const QDateTime &now){
 if(!prayerRestored){prayerRestored=true;auto saved=QDateTime::fromString(store.get("prayer_hold_until"),Qt::ISODate);if(saved>now&&store.get("prayer_pause_enabled")!="0"){prayerUntil=saved;music->suspend(true);priority=(paging||pagePending)?100:60;Q_EMIT prayerPauseChanged(prayerUntil);}}
 if(prayerUntil.isValid()&&now>=prayerUntil){prayerUntil={};store.set("prayer_hold_until","");log("prayer_pause_finished","");Q_EMIT prayerPauseChanged({});if(!paging&&!pagePending&&!cue.active()&&sequenceId.isEmpty()&&speech.state()!=QTextToSpeech::Speaking&&overlay.playbackState()!=QMediaPlayer::PlayingState)release();}
}
