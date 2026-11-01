// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "musictransport.h"
#include "core/player.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistsequence.h"
#include <QFileInfo>
#include <QTimer>
class StrawberryMusic final:public MusicTransport {
 public:
 explicit StrawberryMusic(Player*playerObject,PlaylistManager*managerObject,QObject*parent=nullptr,const QDateTime &heldUntil={}):MusicTransport(parent),player(playerObject),manager(managerObject){
  // Block early file activation without touching the not-yet-initialized engine.
  player->setProperty("camStartupPrayerHoldUntil",heldUntil);
  connect(player,&Player::Playing,this,[this]{
   if(player->property("camLicenseDenied").toBool()){player->Stop();return;}
   if(suspended){suspend(true);return;}
   interruptedRadio.reset();interruptedLocal.reset();interruptedPlaylist=-1;
   const auto item=player->GetCurrentItem();
   Q_EMIT event(QStringLiteral("play_started"),item?item->OriginalUrl().toString():QStringLiteral("strawberry"));
  });
  connect(player,&Player::Stopped,this,[this]{Q_EMIT event(QStringLiteral("play_stopped"),QStringLiteral("strawberry"));});
  connect(player,&Player::Error,this,[this](const QString&e){Q_EMIT event(QStringLiteral("audio_error"),e);});
 }
 bool replace(const QStringList&files)override{
  if(files.isEmpty()||pending)return false;
  QList<QUrl> urls;for(const auto&f:files){if(!QFileInfo::exists(f)||!QFileInfo(f).isFile())return false;urls.append(QUrl::fromLocalFile(f));}
  int id=-1;
  for(int candidate:manager->playlist_ids())if(manager->playlist_name(candidate)==name){id=candidate;break;}
  if(id<0){manager->New(name);id=manager->current_id();}
  auto playlist=manager->playlist(id);if(!playlist)return false;
  interruptedRadio.reset();interruptedLocal.reset();interruptedPlaylist=-1;
  player->Stop();manager->SetCurrentPlaylist(id);manager->SetActivePlaylist(id);
  playlist->Clear();manager->sequence()->SetRepeatMode(PlaylistSequence::RepeatMode::Playlist);
  pending=true;const auto generation=++loadGeneration;
  disconnect(rowsConnection);
  rowsConnection=connect(playlist,&QAbstractItemModel::rowsInserted,this,[this,playlist]{
   if(!pending||playlist->rowCount()==0)return;pending=false;
   if(!suspended)authorizedPlay(true);
  });
  manager->InsertUrls(id,urls,-1,false,false,true);
  QTimer::singleShot(30000,this,[this,generation]{if(pending&&generation==loadGeneration){pending=false;disconnect(rowsConnection);Q_EMIT event(QStringLiteral("audio_error"),QStringLiteral("Strawberry playlist import timed out"));}});
  return true;
 }
 void setGain(double gain)override{requestedGain=gain;player->engine()->SetBusinessGain(suspended?0.0:requestedGain);}
 bool playing()const override{return !suspended&&(player->GetState()==EngineBase::State::Playing||player->engine()->property("nasaqBuffering").toBool()||player->property("nasaqRadioReconnecting").toBool());}
 void suspend(bool active)override{
  player->setProperty("camStartupPrayerHoldUntil",QDateTime());suspended=active;player->setProperty("camSuspended",active);
  // Silence is enforced independently of Pause: GStreamer may ignore Pause while buffering.
  player->engine()->SetBusinessGain(active?0.0:requestedGain);
  if(active){
   const auto item=player->GetCurrentItem();
   if(item&&item->EffectiveMetadata().is_radio()){
    if(!interruptedRadio){interruptedRadio=item;interruptedPlaylist=manager->active_id();}
    // Destroy the live connection without clearing the playlist selection. No fade-out during adhan.
    player->engine()->Stop(true);
   }else{if(!interruptedLocal)interruptedLocal=item;player->Pause();}
  }
 }
 void resume()override{
  if(suspended||pending)return;
  if(interruptedLocal){
   const auto item=interruptedLocal;interruptedLocal.reset();
   // Main-window Stop also cancels a local resume, just as it does for radio.
   if(player->GetCurrentItem()!=item)return;
  }
  if(interruptedRadio){
   const auto item=interruptedRadio;interruptedRadio.reset();
   // A manual Stop clears Player's item while an adhan engine-only stop preserves it.
   if(player->GetCurrentItem()!=item){interruptedPlaylist=-1;return;}
   auto playlist=manager->playlist(interruptedPlaylist);interruptedPlaylist=-1;
   if(!playlist)return;
   for(int row=0;row<playlist->rowCount();++row)if(playlist->item_at(row)==item){
    manager->SetActivePlaylist(playlist->id());
    player->setProperty("camAuthorized",true);
    player->PlayAt(row,false,0,EngineBase::TrackChangeType::First,Playlist::AutoScroll::Maybe,false);
    player->setProperty("camAuthorized",false);return;
   }
   return;
  }
  authorizedPlay(false);
 }
 void stop()override{pending=false;++loadGeneration;disconnect(rowsConnection);interruptedRadio.reset();interruptedLocal.reset();interruptedPlaylist=-1;player->Stop();}
 private:
 void authorizedPlay(bool playlist){player->setProperty("camAuthorized",true);if(playlist)player->PlayPlaylist(name);else player->Play();player->setProperty("camAuthorized",false);}
 Player*player;PlaylistManager*manager;bool suspended=false,pending=false;quint64 loadGeneration=0;QMetaObject::Connection rowsConnection;
 double requestedGain=1.0;PlaylistItemPtr interruptedRadio,interruptedLocal;int interruptedPlaylist=-1;
 const QString name=QStringLiteral("NASAQ — Business playlist");
};

