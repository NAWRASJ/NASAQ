// SPDX-License-Identifier: GPL-3.0-or-later
#include "strawberry_cam.h"
#include "audience.h"
#include "covermanager/currentalbumcoverloader.h"
#include "engine/gstengine.h"
#include <QMediaDevices>
#include <QStatusBar>
#include "musictransport.h"
#include "window.h"
#include "nasaqtheme.h"
#include <QJsonArray>
#include "core/application.h"
#include "core/player.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistsequence.h"
#include <QStandardPaths>
#include <QMenuBar>
#include <QMenu>
#include "silentmessagebox.h"
#include <QFileInfo>
#include <QTimer>
#include <QPushButton>
#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <QToolBar>
#include <QMimeData>
#include <QUndoStack>
#include "playlist/playlistview.h"
#include <QSpinBox>
#include <QHBoxLayout>
#include "rundown.h"
#include "queue/queue.h"
#include <stdexcept>
#include <cmath>

// Owns music through Strawberry's Player and PlaylistManager. No second music player.
// Qt provides only foreground announcement, SAPI and live microphone services.
class StrawberryMusic final:public MusicTransport {
 public:
 explicit StrawberryMusic(Application*application,QObject*parent=nullptr,const QDateTime &heldUntil={}):MusicTransport(parent),app(application){
  // Block early file activation without touching the not-yet-initialized engine.
  app->player()->setProperty("camStartupPrayerHoldUntil",heldUntil);
  connect(&*app->player(),&Player::Playing,this,[this]{
   if(app->player()->property("camLicenseDenied").toBool()){app->player()->Stop();return;}
   if(suspended){suspend(true);return;}
   interruptedRadio.reset();interruptedPlaylist=-1;
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
  interruptedRadio.reset();interruptedPlaylist=-1;
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
 void setGain(double gain)override{requestedGain=gain;app->player()->engine()->SetBusinessGain(suspended?0.0:requestedGain);}
 bool playing()const override{return !suspended&&(app->player()->GetState()==EngineBase::State::Playing||app->player()->engine()->property("nasaqBuffering").toBool()||app->player()->property("nasaqRadioReconnecting").toBool());}
 void suspend(bool active)override{
  app->player()->setProperty("camStartupPrayerHoldUntil",QDateTime());suspended=active;app->player()->setProperty("camSuspended",active);
  // Silence is enforced independently of Pause: GStreamer may ignore Pause while buffering.
  app->player()->engine()->SetBusinessGain(active?0.0:requestedGain);
  if(active){
   const auto item=app->player()->GetCurrentItem();
   if(item&&item->EffectiveMetadata().is_radio()){
    if(!interruptedRadio){interruptedRadio=item;interruptedPlaylist=app->playlist_manager()->active_id();}
    // Destroy the live connection without clearing the playlist selection. No fade-out during adhan.
    app->player()->engine()->Stop(true);
   }else app->player()->Pause();
  }
 }
 void resume()override{
  if(suspended||pending)return;
  if(interruptedRadio){
   const auto item=interruptedRadio;interruptedRadio.reset();
   // A manual Stop clears Player's item while an adhan engine-only stop preserves it.
   if(app->player()->GetCurrentItem()!=item){interruptedPlaylist=-1;return;}
   auto playlist=app->playlist_manager()->playlist(interruptedPlaylist);interruptedPlaylist=-1;
   if(!playlist)return;
   for(int row=0;row<playlist->rowCount();++row)if(playlist->item_at(row)==item){
    app->playlist_manager()->SetActivePlaylist(playlist->id());
    app->player()->setProperty("camAuthorized",true);
    app->player()->PlayAt(row,false,0,EngineBase::TrackChangeType::First,Playlist::AutoScroll::Maybe,false);
    app->player()->setProperty("camAuthorized",false);return;
   }
   return;
  }
  authorizedPlay(false);
 }
 void stop()override{pending=false;++loadGeneration;disconnect(rowsConnection);interruptedRadio.reset();interruptedPlaylist=-1;app->player()->Stop();}
 private:
 void authorizedPlay(bool playlist){app->player()->setProperty("camAuthorized",true);if(playlist)app->player()->PlayPlaylist(name);else app->player()->Play();app->player()->setProperty("camAuthorized",false);}
 Application*app;bool suspended=false,pending=false;quint64 loadGeneration=0;QMetaObject::Connection rowsConnection;
 double requestedGain=1.0;PlaylistItemPtr interruptedRadio;int interruptedPlaylist=-1;
 const QString name=QStringLiteral("NASAQ — Business playlist");
};

class CamHost final:public QObject {
 public:
 CamHost(Application*app,QMainWindow*main):QObject(main){
  QTimer::singleShot(0,main,[]{ApplyNasaqTheme();});
  const QString dir=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/CAM");
  if(!QDir().mkpath(dir))throw std::runtime_error("Cannot create NASAQ data directory");
  store=std::make_unique<Store>(dir+QStringLiteral("/cam.sqlite"));
  auto strip=new QToolBar(main);strip->setObjectName(QStringLiteral("businessDuration"));strip->setMovable(false);main->addToolBar(Qt::TopToolBarArea,strip);
  auto prayerState=new QLabel(main);prayerState->setObjectName(QStringLiteral("prayerReadinessBanner"));prayerState->setWordWrap(true);strip->addWidget(prayerState);
  auto prayerTimer=new QTimer(this);auto updatePrayer=[this,prayerState]{
   const bool ar=NasaqLanguage()==QStringLiteral("ar");const bool enabled=store->get(QStringLiteral("prayer_pause_enabled"))!=QStringLiteral("0");
   const auto now=QDateTime::currentDateTimeUtc();
   // A timetable remains useful even when automatic music pauses are disabled.
   auto query=store->query(QStringLiteral("SELECT due FROM events WHERE kind IN ('prayer','adhan') AND status='pending' AND due>=? ORDER BY due LIMIT 1"),{now.toMSecsSinceEpoch()});
   const QDateTime next=enabled?PrayerService::nextPause(*store,now):(query.next()?QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong(),QTimeZone::UTC):QDateTime());
   const auto hold=QDateTime::fromString(store->get(QStringLiteral("prayer_hold_until")),Qt::ISODate);
   const QString prefix=enabled?(ar?QStringLiteral("التوقف للأذان القادم: "):QStringLiteral("Next adhan pause: ")):(ar?QStringLiteral("موعد الأذان القادم: "):QStringLiteral("Next adhan time: "));
   QString message;
   if(enabled&&hold>now)message=(ar?QStringLiteral("التوقف للأذان جارٍ حتى "):QStringLiteral("Adhan pause until "))+hold.toLocalTime().toString(QStringLiteral("HH:mm"));
   else if(next.isValid())message=prefix+next.toLocalTime().toString(QStringLiteral("ddd HH:mm t"));
   else message=prefix+(ar?QStringLiteral("لا تتوفر مواقيت — افتح أوقات الأذان"):QStringLiteral("No timetable — open Adhan times"));
   if(enabled&&hold<=now&&next.isValid()&&next>now){
    const qint64 seconds=now.secsTo(next);
    const QString remaining=QStringLiteral("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0')).arg((seconds/60)%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
    message+=(ar?QStringLiteral("\nمتبقي للتوقف: "):QStringLiteral("\nPause in: "))+remaining;
   }
   prayerState->setText(message);
   prayerState->setProperty("audienceCaption",AudienceDisplay::prayerCaption(*store,now));
   prayerState->setStyleSheet(enabled?QStringLiteral("color:#b93037;padding:8px;font-weight:600"):QStringLiteral("color:#365d49;padding:8px"));
  };connect(prayerTimer,&QTimer::timeout,this,updatePrayer);prayerTimer->start(1000);updatePrayer();
  auto panel=new QWidget(strip);auto row=new QHBoxLayout(panel);row->setContentsMargins(8,8,8,8);row->setSpacing(16);auto total=new QLabel(panel);total->setObjectName(QStringLiteral("playlistTotalDuration"));total->setStyleSheet(QStringLiteral("font-size:24px;font-weight:600;color:#153e34;padding:8px"));auto coverage=new QLabel(panel);coverage->setObjectName(QStringLiteral("coverageSummary"));coverage->setWordWrap(true);auto target=new QSpinBox(panel);target->setRange(1,1440);target->setValue(store->get(QStringLiteral("coverage_minutes")).isEmpty()?480:store->get(QStringLiteral("coverage_minutes")).toInt());bool ar=NasaqLanguage()==QStringLiteral("ar");target->setSuffix(ar?QStringLiteral(" دقيقة عمل"):QStringLiteral(" business minutes"));row->addWidget(coverage,1);row->addWidget(target);row->addWidget(total);strip->addWidget(panel);panel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  connect(target,&QSpinBox::valueChanged,this,[this](int minutes){store->set(QStringLiteral("coverage_minutes"),QString::number(minutes));});
  auto clock=new QTimer(this);connect(clock,&QTimer::timeout,this,[this,app,total,coverage,target]{
   const bool ar=NasaqLanguage()==QStringLiteral("ar");target->setSuffix(ar?QStringLiteral(" دقيقة عمل"):QStringLiteral(" business minutes"));
   auto list=app->playlist_manager()->current();if(!list)return;QList<qint64> durations;QSet<int> liveRows;bool skipped=false;for(int i=0;i<list->rowCount();++i){auto item=list->item_at(i);durations<<item->EffectiveMetadata().length_nanosec()/1000000;if(item->EffectiveMetadata().is_radio())liveRows.insert(i);skipped|=item->GetShouldSkip();}
   auto seq=app->playlist_manager()->sequence();bool ordered=seq->shuffle_mode()==PlaylistSequence::ShuffleMode::Off&&(seq->repeat_mode()==PlaylistSequence::RepeatMode::Off||seq->repeat_mode()==PlaylistSequence::RepeatMode::Playlist)&&list->queue()->rowCount()==0&&!skipped;
   bool active=list==app->playlist_manager()->active()&&app->player()->GetState()!=EngineBase::State::Empty&&app->player()->GetState()!=EngineBase::State::Idle;
   auto result=CalculateRundown(durations,active?list->current_row():0,active?app->player()->engine()->position_nanosec()/1000000:0,QDateTime::currentDateTime(),ordered,liveRows);list->SetBusinessEndTimes(result.ends);
   total->setText((ar?QStringLiteral("إجمالي القائمة  "):QStringLiteral("Playlist total  "))+(result.infinite?QStringLiteral("∞"):RundownDuration(result.total)));
   total->setToolTip(result.infinite?(ar?QStringLiteral("تحتوي القائمة على بث راديو مباشر بلا مدة نهائية"):QStringLiteral("This playlist includes live radio with no fixed duration")):QString());
   target->setVisible(!result.infinite);
   if(app->player()->property("nasaqRadioReconnecting").toBool()){
    coverage->setText(ar?QStringLiteral("انقطع اتصال الراديو — جارٍ إعادة الاتصال تلقائيًا…\nيمكنك الإيقاف أو اختيار محطة أخرى. يبقى التوقف للأذان فعالًا وفق إعداداتك."):QStringLiteral("Radio disconnected — reconnecting automatically…\nYou can stop or choose another station. Adhan pauses still follow your settings."));return;
   }
   if(result.infinite){const bool pauseEnabled=store->get(QStringLiteral("prayer_pause_enabled"))!=QStringLiteral("0");coverage->setText(ar?(pauseEnabled?QStringLiteral("مدة غير محدودة\nيتوقف للأذان ويستأنف وفق إعداداتك؛ يعتمد استمرار البث على اتصال المحطة."):QStringLiteral("مدة غير محدودة\nإيقاف الموسيقى للأذان غير مفعل؛ يعتمد استمرار البث على اتصال المحطة.")):(pauseEnabled?QStringLiteral("No fixed duration\nAdhan pauses still apply. Broadcast availability depends on the station connection."):QStringLiteral("No fixed duration\nAutomatic adhan pause is off. Broadcast availability depends on the station connection.")));return;}
   qint64 missing=qMax(qint64(0),qint64(target->value())*60000-result.total);
   QString text=(ar?QStringLiteral("المتبقي: "):QStringLiteral("Remaining: "))+RundownDuration(result.remaining)+(ar?QStringLiteral(" • نقص التغطية: "):QStringLiteral(" • Coverage shortfall: "))+RundownDuration(missing);
   if(result.unknown)text+=ar?QStringLiteral(" • توجد مدد غير معروفة"):QStringLiteral(" • Unknown durations present");
   text+=ordered?(ar?QStringLiteral("\nالانتهاء تقديري حسب ساعة الكمبيوتر؛ يتغير عند التوقف والإعلانات."):QStringLiteral("\nEnd times use the PC clock; pauses and announcements shift estimates.")):(ar?QStringLiteral("\nأوقات الانتهاء تتطلب ترتيبًا متسلسلًا دون تخطي أو قائمة انتظار."):QStringLiteral("\nEnd times require sequential order without skips or queued tracks."));coverage->setText(text);
  });clock->start(1000);
  music=std::make_unique<StrawberryMusic>(app,nullptr,store->get(QStringLiteral("prayer_pause_enabled"))!=QStringLiteral("0") ? QDateTime::fromString(store->get(QStringLiteral("prayer_hold_until")),Qt::ISODate) : QDateTime());
  window=std::make_unique<Window>(*store,music.get(),main);
  audience=std::make_unique<AudienceDisplay>(*store,main);
  connect(&*app->player(),&Player::ManualMediaRequested,audience.get(),&AudienceDisplay::manualMediaRequested);
  connect(audience.get(),&AudienceDisplay::playPauseRequested,&*app->player(),&Player::PlayPauseHelper);
  connect(audience.get(),&AudienceDisplay::stopRequested,this,[app]{app->player()->Stop();});
  auto adjacentStation=[app](int direction){
   auto list=app->playlist_manager()->active();if(!list)return -1;
   for(int row=list->current_row()+direction;row>=0&&row<list->rowCount();row+=direction)
    if(list->item_at(row)->EffectiveMetadata().is_radio())return row;
   return -1;
  };
  auto navigate=[app,adjacentStation](int direction){
   const auto item=app->player()->GetCurrentItem();
   if(item&&item->EffectiveMetadata().is_radio()){
    const int row=adjacentStation(direction);if(row>=0)app->player()->PlayAt(row,false,0,EngineBase::TrackChangeType::Manual,Playlist::AutoScroll::Always,false);
   }else if(direction<0)app->player()->Previous();else app->player()->Next();
  };
  connect(audience.get(),&AudienceDisplay::previousRequested,this,[navigate]{navigate(-1);});
  connect(audience.get(),&AudienceDisplay::volumeRequested,this,[app](int volume){app->player()->SetVolume(volume);});
  connect(audience.get(),&AudienceDisplay::nextRequested,this,[navigate]{navigate(1);});
  connect(audience.get(),&AudienceDisplay::seekRequested,&*app->player(),&Player::SeekTo);
  connect(&*app->playlist_manager(),&PlaylistManager::CurrentSongChanged,audience.get(),[this](const Song &song){audience->beginTrack(song.url().toString());});
  connect(&*app->player(),&Player::Stopped,audience.get(),[this]{audience->beginTrack({});});
  connect(&*app->current_albumcover_loader(),&CurrentAlbumCoverLoader::AlbumCoverLoaded,audience.get(),[this,app](const Song &song,const AlbumCoverLoaderResult &result){
   const auto item=app->player()->GetCurrentItem();if(!item||item->EffectiveMetadata().url()!=song.url())return;
   const bool genuine=result.success&&result.type!=AlbumCoverLoaderResult::Type::None&&result.type!=AlbumCoverLoaderResult::Type::Unset&&!song.is_radio();
   audience->setAlbumCover(song.url().toString(),result.album_cover.image,genuine);
  });
  connect(window.get(),&Window::interfaceLanguageChanged,audience.get(),&AudienceDisplay::updateLanguage);
  connect(audience.get(),&AudienceDisplay::audioSettingsRequested,this,[main]{if(auto action=main->findChild<QAction*>(QStringLiteral("action_settings")))action->trigger();});
  auto displayStop=[this,app,main]{
   app->player()->Stop();
   main->statusBar()->showMessage(NasaqLanguage()==QStringLiteral("ar")?QStringLiteral("توقف التشغيل بسبب تغيير شاشة الجمهور أو جهاز الصوت. تحقق من التوصيلات ثم استأنف يدوياً."):QStringLiteral("Playback stopped after a display or audio-device change. Check connections and resume manually."),15000);
  };
  connect(audience.get(),&AudienceDisplay::safetyStopRequested,this,displayStop);
  auto devices=new QMediaDevices(this);connect(devices,&QMediaDevices::audioOutputsChanged,this,[this,displayStop]{if(audience->externalEnabled())displayStop();});
  auto videoTimer=new QTimer(this);connect(videoTimer,&QTimer::timeout,this,[this,app,prayerState,adjacentStation]{
   auto engine=qobject_cast<GstEngine*>(&*app->player()->engine());if(!engine)return;
   const bool prayer=window->prayerActive();
   const bool blocked=prayer||!window->licenseAllowed()||app->player()->property("camSuspended").toBool()||app->player()->GetState()==EngineBase::State::Empty||app->player()->GetState()==EngineBase::State::Idle||app->player()->GetState()==EngineBase::State::Error;
   audience->updateFrame(blocked?QImage():engine->TakeVideoFrame(),engine->VideoGeneration(),prayer,blocked);
   const bool playing=!blocked&&app->player()->GetState()==EngineBase::State::Playing;
   const bool reconnecting=app->player()->property("nasaqRadioReconnecting").toBool()||engine->property("nasaqBuffering").toBool();
   const auto item=app->player()->GetCurrentItem();QVector<float> bands;
   if(playing&&!reconnecting){
    const auto pcm=engine->display_scope();const int frames=qMin(512,int(pcm.size()/2));
    if(frames>0){bands.resize(24);for(int b=0;b<24;++b){double re=0,im=0;const double bin=1+std::pow(1.24,b);
     for(int i=0;i<frames;++i){const double sample=(double(pcm[i*2])+pcm[i*2+1])/65536.*(.5-.5*std::cos(6.283185307*i/qMax(1,frames-1)));const double a=6.283185307*bin*i/frames;re+=sample*std::cos(a);im+=sample*std::sin(a);}
     bands[b]=float(qBound(0.,std::log1p(std::sqrt(re*re+im*im)/frames*60)/std::log(16.),1.));
    }}
   }
   audience->setPlayback(playing,item&&item->EffectiveMetadata().is_radio(),reconnecting,item?item->EffectiveMetadata().PrettyTitle():QString(),prayerState->property("audienceCaption").toString(),bands);
   audience->preview()->setVolume(app->player()->GetVolume());
   const bool radio=item&&item->EffectiveMetadata().is_radio();
   audience->preview()->setNavigation(!radio||adjacentStation(-1)>=0,!radio||adjacentStation(1)>=0);
   audience->setTimeline(engine->position_nanosec()/1000000000,engine->length_nanosec()/1000000000);

  });videoTimer->start(40);

  const auto sectionNames=Window::sectionNames();
  for(int i=0;i<sectionNames.size();++i){
   auto action=main->menuBar()->addAction(sectionNames[i]);action->setObjectName(QStringLiteral("nasaqSection%1").arg(i));
   connect(action,&QAction::triggered,this,[this,i]{window->openSection(i);});
  }
  connect(window.get(),&Window::interfaceLanguageChanged,this,[this,main]{
   main->setWindowTitle(NText(QStringLiteral("NASAQ"))+QStringLiteral(" " NASAQ_VERSION_STRING));
   for(int i=0;i<Window::sectionNames().size();++i)if(auto action=main->findChild<QAction*>(QStringLiteral("nasaqSection%1").arg(i)))action->setText(Window::sectionNames()[i]);
  });
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
  const QString businessTest=qEnvironmentVariable("NASAQ_BUSINESS_TEST_OUTPUT");
  if(!businessTest.isEmpty()&&!qEnvironmentVariableIsEmpty("CAM_SMOKE_OUTPUT")){
   if(QCoreApplication::applicationName().startsWith(QStringLiteral("NASAQ-PersistenceTest-"))&&qEnvironmentVariable("NASAQ_PERSIST_TEST_PHASE")==QStringLiteral("verify")){
    QTimer::singleShot(5000,this,[this,app,main,businessTest]{
     auto list=app->playlist_manager()->current();QJsonObject result;
     result.insert(QStringLiteral("restoredThreeTracks"),list&&list->rowCount()==3);
     bool order=list&&list->rowCount()==3;if(order){QStringList expected{QStringLiteral("Business test 2"),QStringLiteral("Business test 3"),QStringLiteral("Business test 1")};for(int i=0;i<3;++i)order&=list->item_at(i)->EffectiveMetadata().title()==expected[i];}
     result.insert(QStringLiteral("restoredExactOrder"),order);result.insert(QStringLiteral("noAutomaticPlayback"),!music->playing());result.insert(QStringLiteral("passed"),order&&list&&list->rowCount()==3&&!music->playing());
     QFile file(businessTest+QStringLiteral(".json"));if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(result).toJson());QMetaObject::invokeMethod(main,"Exit",Qt::QueuedConnection);
    });return;
   }
   QTimer::singleShot(3000,this,[this,app,main,businessTest]{
    app->playlist_manager()->New(QStringLiteral("NASAQ business verification"));auto list=app->playlist_manager()->current();if(!list)return;SongList songs;for(int i=0;i<3;++i){Song song;song.Init(QStringLiteral("Business test %1").arg(i+1),QStringLiteral("NASAQ"),QStringLiteral("Timing verification"),qint64(i+1)*60000000000LL);songs<<song;}list->InsertSongs(songs);list->set_current_row(0);auto original=list->item_at(0);std::unique_ptr<QMimeData> mime(list->mimeData({list->index(0,0)}));bool dropped=list->dropMimeData(mime.get(),Qt::MoveAction,3,0,{});
    testResults.insert(QStringLiteral("dragDropMovesSong"),dropped&&list->item_at(2)==original);testResults.insert(QStringLiteral("currentSongPreserved"),list->current_row()==2);list->undo_stack()->undo();testResults.insert(QStringLiteral("undoRestoresOrder"),list->item_at(0)==original);list->undo_stack()->redo();testResults.insert(QStringLiteral("redoRestoresMove"),list->item_at(2)==original);list->set_current_row(-1);app->playlist_manager()->sequence()->SetShuffleMode(PlaylistSequence::ShuffleMode::Off);app->playlist_manager()->sequence()->SetRepeatMode(PlaylistSequence::RepeatMode::Off);
    auto view=main->findChild<PlaylistView*>();testResults.insert(QStringLiteral("dragEnabled"),view&&view->dragEnabled()&&view->acceptDrops()&&view->defaultDropAction()==Qt::MoveAction);
    QTimer::singleShot(1500,this,[this,main,list,businessTest]{auto label=main->findChild<QLabel*>(QStringLiteral("playlistTotalDuration"));testResults.insert(QStringLiteral("totalSixMinutes"),label&&label->text().contains(QStringLiteral("00:06:00")));testResults.insert(QStringLiteral("endColumnPopulated"),!list->data(list->index(0,static_cast<int>(Playlist::Column::BusinessEnd))).toString().isEmpty());window->hide();main->grab().save(businessTest+QStringLiteral(".png"));bool passed=true;for(auto value:testResults)passed&=value.toBool();testResults.insert(QStringLiteral("passed"),passed);QFile out(businessTest+QStringLiteral(".json"));if(out.open(QIODevice::WriteOnly))out.write(QJsonDocument(testResults).toJson());QMetaObject::invokeMethod(main,"Exit",Qt::QueuedConnection);});
   });return;
  }
  const QString liveTest=qEnvironmentVariable("NASAQ_LIVE_TEST_OUTPUT");
  if(!liveTest.isEmpty()){
   auto finish=[this,main,app,liveTest](bool timeout){bool promptVisible=false;for(auto *dialog:window->findChildren<QDialog*>())if(dialog->objectName()==QStringLiteral("activationDialog")&&dialog->isVisible())promptVisible=true;QJsonObject result{{QStringLiteral("activationPromptVisible"),promptVisible},{QStringLiteral("licensed"),window->licenseAllowed()},{QStringLiteral("playing"),music->playing()},{QStringLiteral("positionNs"),double(app->player()->engine()->position_nanosec())},{QStringLiteral("timeout"),timeout}};QFile file(liveTest);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(result).toJson());music->stop();QMetaObject::invokeMethod(main,"Exit",Qt::QueuedConnection);};
   auto start=[this,finish]{if(property("liveTestStarted").toBool())return;setProperty("liveTestStarted",true);music->replace({qEnvironmentVariable("CAM_TEST_WAV")});QTimer::singleShot(12000,this,[finish]{finish(false);});};
   connect(window.get(),&Window::licenseAllowedChanged,this,[start](bool allowed){if(allowed)start();});
   QTimer::singleShot(1000,this,[this,start]{if(window->licenseAllowed())start();else window->activateLicense(qEnvironmentVariable("NASAQ_ACTIVATION_TEST_KEY"));});
   QTimer::singleShot(50000,this,[finish]{finish(true);});return;
  }
  const QString smoke=qEnvironmentVariable("CAM_SMOKE_OUTPUT");
  if(!smoke.isEmpty()&&!window->licenseAllowed()){
   window->show();
   if(!qEnvironmentVariable("NASAQ_UI_SMOKE").isEmpty())QTimer::singleShot(2200,this,[this]{for(auto *b:window->findChildren<QPushButton*>())if(b->text()==NText(QStringLiteral("Activate"))){b->click();break;}});
   QTimer::singleShot(1500,this,[this]{music->replace({qEnvironmentVariable("CAM_TEST_WAV")});});
   QTimer::singleShot(3500,this,[this,app]{testResults.insert(QStringLiteral("playlistPlaybackBlocked"),!music->playing());app->player()->PlayPause();});
   QTimer::singleShot(4500,this,[this,main,smoke]{
    testResults.insert(QStringLiteral("manualPlaybackBlocked"),!music->playing());
    testResults.insert(QStringLiteral("unactivated"),!window->licenseAllowed());music->stop();
    testResults.insert(QStringLiteral("screenshotSaved"),window->grab().save(smoke+QStringLiteral(".png")));
    main->grab().save(smoke+QStringLiteral("-player.png"));
    QJsonArray labels;for(auto *label:window->findChildren<QLabel*>())labels.append(label->text());for(auto *button:window->findChildren<QPushButton*>())labels.append(button->text());
    QJsonObject ui{{QStringLiteral("language"),NasaqLanguage()},{QStringLiteral("rtl"),window->layoutDirection()==Qt::RightToLeft},{QStringLiteral("labels"),labels}};
    QFile uiOutput(smoke+QStringLiteral("-ui.json"));if(uiOutput.open(QIODevice::WriteOnly))uiOutput.write(QJsonDocument(ui).toJson());
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
    main->grab().save(smoke+QStringLiteral("-player.png"));
    QJsonArray labels;for(auto *label:window->findChildren<QLabel*>())labels.append(label->text());for(auto *button:window->findChildren<QPushButton*>())labels.append(button->text());
    QJsonObject ui{{QStringLiteral("language"),NasaqLanguage()},{QStringLiteral("rtl"),window->layoutDirection()==Qt::RightToLeft},{QStringLiteral("labels"),labels}};
    QFile uiOutput(smoke+QStringLiteral("-ui.json"));if(uiOutput.open(QIODevice::WriteOnly))uiOutput.write(QJsonDocument(ui).toJson());
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
 std::unique_ptr<AudienceDisplay> audience;
 QJsonObject testResults;
};
void AttachCam(Application*app,QMainWindow*main){
 app->player()->setProperty("camLicenseDenied",true);
 try{new CamHost(app,main);}catch(const std::exception&e){main->setEnabled(true);app->player()->setProperty("camOperatorLocked",false);app->player()->setProperty("camSuspended",false);SilentMessageBox::critical(main,QStringLiteral("NASAQ startup failed"),QString::fromUtf8(e.what()));}
}
