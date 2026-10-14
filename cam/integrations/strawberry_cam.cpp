// SPDX-License-Identifier: GPL-3.0-or-later
#include "strawberry_cam.h"
#include "musictransport.h"
#include "window.h"
#include "core/application.h"
#include "core/player.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistsequence.h"
#include <QStandardPaths>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <stdexcept>

// Owns music through Strawberry's Player and PlaylistManager. No second music player.
// Qt provides only foreground announcement, SAPI and live microphone services.
class StrawberryMusic final:public MusicTransport {
 public:
 explicit StrawberryMusic(Application*application,QObject*parent=nullptr):MusicTransport(parent),app(application){
  connect(&*app->player(),&Player::Playing,this,[this]{
   if(app->player()->property("camLicenseDenied").toBool()){app->player()->Stop();return;}
   if(suspended){app->player()->Pause();return;}
   const auto item=app->player()->GetCurrentItem();
   Q_EMIT event(QStringLiteral("play_started"),item?item->OriginalUrl().toString():QStringLiteral("strawberry"));
  });
  connect(&*app->player(),&Player::Stopped,this,[this]{Q_EMIT event(QStringLiteral("play_stopped"),QStringLiteral("strawberry"));});
  connect(&*app->player(),&Player::Error,this,[this](const QString&e){Q_EMIT event(QStringLiteral("audio_error"),e);});
 }
 bool replace(const QStringList&files)override{
  if(files.isEmpty()||pending)return false;
  QList<QUrl> urls;for(const auto&f:files){if(!QFileInfo::exists(f)||!QFileInfo(f).isFile())return false;urls.append(QUrl::fromLocalFile(f));}
  auto manager=app->playlist_manager();int id=-1;
  for(int candidate:manager->playlist_ids())if(manager->playlist_name(candidate)==name){id=candidate;break;}
  if(id<0){manager->New(name);id=manager->current_id();}
  auto playlist=manager->playlist(id);if(!playlist)return false;
  app->player()->Stop();manager->SetCurrentPlaylist(id);manager->SetActivePlaylist(id);
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
 bool playing()const override{return app->player()->GetState()==EngineBase::State::Playing;}
 void suspend(bool active)override{suspended=active;app->player()->setProperty("camSuspended",active);if(active)app->player()->Pause();}
 void resume()override{if(!suspended&&!pending)authorizedPlay(false);}
 void stop()override{pending=false;++loadGeneration;disconnect(rowsConnection);app->player()->Stop();}
 private:
 void authorizedPlay(bool playlist){app->player()->setProperty("camAuthorized",true);if(playlist)app->player()->PlayPlaylist(name);else app->player()->Play();app->player()->setProperty("camAuthorized",false);}
 Application*app;bool suspended=false,pending=false;quint64 loadGeneration=0;QMetaObject::Connection rowsConnection;
 const QString name=QStringLiteral("NASAQ — Business playlist");
};

class CamHost final:public QObject {
 public:
 CamHost(Application*app,QMainWindow*main):QObject(main){
  const QString dir=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/CAM");
  if(!QDir().mkpath(dir))throw std::runtime_error("Cannot create NASAQ data directory");
  store=std::make_unique<Store>(dir+QStringLiteral("/cam.sqlite"));
  music=std::make_unique<StrawberryMusic>(app);
  window=std::make_unique<Window>(*store,music.get());
  window->setWindowTitle(QStringLiteral("نسق NASAQ 0.3.0 — إدارة الصوت التجاري"));
  auto action=main->menuBar()->addAction(QStringLiteral("نسق NASAQ / إدارة الصوت"));
  connect(action,&QAction::triggered,this,[this]{window->show();window->raise();window->activateWindow();});
  auto lockControls=[app,main](bool locked){main->setEnabled(!locked);app->player()->setProperty("camOperatorLocked",locked);};
  connect(window.get(),&Window::operatorLockChanged,this,lockControls);
  lockControls(window->operatorLocked());
  auto licenseControls=[app](bool allowed){app->player()->setProperty("camLicenseDenied",!allowed);if(!allowed)app->player()->Stop();};
  connect(window.get(),&Window::licenseAllowedChanged,this,licenseControls);
  // Gate immediately; do not call Stop before MainWindow initializes the engine.
  app->player()->setProperty("camLicenseDenied",!window->licenseAllowed());
  if(!window->licenseAllowed())window->show();
  if(window->operatorLocked())window->show();
  // Scheduling remains active when its window is hidden; the host lasts until exit.
  store->log(QStringLiteral("integration"),QStringLiteral("Strawberry in-process music transport attached"));
  const QString smoke=qEnvironmentVariable("CAM_SMOKE_OUTPUT");
  if(!smoke.isEmpty()&&!window->licenseAllowed()){
   window->show();
   QTimer::singleShot(1500,this,[this]{music->replace({qEnvironmentVariable("CAM_TEST_WAV")});});
   QTimer::singleShot(3500,this,[this,app]{testResults.insert(QStringLiteral("playlistPlaybackBlocked"),!music->playing());app->player()->PlayPause();});
   QTimer::singleShot(4500,this,[this,main,smoke]{
    testResults.insert(QStringLiteral("manualPlaybackBlocked"),!music->playing());
    testResults.insert(QStringLiteral("unactivated"),!window->licenseAllowed());music->stop();
    testResults.insert(QStringLiteral("screenshotSaved"),window->grab().save(smoke+QStringLiteral(".png")));
    bool passed=true;for(const auto&value:testResults)passed=passed&&value.toBool();testResults.insert(QStringLiteral("passed"),passed);
    QFile output(smoke+QStringLiteral(".json"));if(output.open(QIODevice::WriteOnly))output.write(QJsonDocument(testResults).toJson());
    QMetaObject::invokeMethod(main,"Exit",Qt::QueuedConnection);
   });
   return;
  }
  if(!smoke.isEmpty()){
   window->show();
   QTimer::singleShot(3000,this,[this]{testResults.insert(QStringLiteral("playlistAccepted"),music->replace({qEnvironmentVariable("CAM_TEST_WAV")}));});
   QTimer::singleShot(6000,this,[this]{testResults.insert(QStringLiteral("musicPlaying"),music->playing());music->suspend(true);});
   QTimer::singleShot(7000,this,[this,app]{testResults.insert(QStringLiteral("musicPaused"),!music->playing());app->player()->PlayPause();});
   QTimer::singleShot(8000,this,[this]{testResults.insert(QStringLiteral("manualResumeBlocked"),!music->playing());music->suspend(false);music->resume();});
   QTimer::singleShot(10000,this,[this]{testResults.insert(QStringLiteral("musicResumed"),music->playing());music->stop();});
   QTimer::singleShot(11000,this,[this,main,smoke]{
    testResults.insert(QStringLiteral("musicStopped"),!music->playing());
    testResults.insert(QStringLiteral("screenshotSaved"),window->grab().save(smoke+QStringLiteral(".png")));
    bool passed=true;for(const auto&value:testResults)passed=passed&&value.toBool();testResults.insert(QStringLiteral("passed"),passed);
    QFile output(smoke+QStringLiteral(".json"));if(output.open(QIODevice::WriteOnly))output.write(QJsonDocument(testResults).toJson());
    QMetaObject::invokeMethod(main,"Exit",Qt::QueuedConnection);
   });
  }
 }
 private:
 std::unique_ptr<Store> store;
 std::unique_ptr<StrawberryMusic> music;
 std::unique_ptr<Window> window;
 QJsonObject testResults;
};
void AttachCam(Application*app,QMainWindow*main){
 app->player()->setProperty("camLicenseDenied",true);
 try{new CamHost(app,main);}catch(const std::exception&e){main->setEnabled(true);app->player()->setProperty("camOperatorLocked",false);app->player()->setProperty("camSuspended",false);QMessageBox::critical(main,QStringLiteral("NASAQ startup failed"),QString::fromUtf8(e.what()));}
}
