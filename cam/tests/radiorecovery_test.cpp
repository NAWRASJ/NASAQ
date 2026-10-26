// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "core/player.h"
#include "core/taskmanager.h"
#include "core/urlhandlers.h"
#include "core/settings.h"
#include "engine/gstengine.h"
#include "engine/gstenginepipeline.h"
#include "playlist/playlistmanager.h"
#include "playlist/songplaylistitem.h"
using std::make_shared;

class RecoveryManager final : public PlaylistManager {
 public:
  RecoveryManager():PlaylistManager(nullptr,nullptr,nullptr,nullptr,nullptr,nullptr),list(nullptr,nullptr,nullptr,nullptr,nullptr,1){}
  Playlist list;
  Playlist *active() const override { return const_cast<Playlist*>(&list); }
  Playlist *current() const override { return active(); }
  int active_id() const override { return 1; }
  int current_id() const override { return 1; }
};

class RecoveryEngine final : public EngineBase {
 public:
  State state_=State::Empty; int plays=0; quint64 offset=99;
  bool Init() override { return true; }
  State state() const override { return state_; }
  bool Play(bool pause,quint64 position) override {++plays;offset=position;state_=pause?State::Paused:State::Playing;Q_EMIT StateChanged(state_);return true;}
  void Stop(bool=false) override {state_=State::Empty;Q_EMIT StateChanged(state_);}
  void Pause() override {state_=State::Paused;Q_EMIT StateChanged(state_);}
  void Unpause() override {state_=State::Playing;Q_EMIT StateChanged(state_);}
  void Seek(quint64) override {}
  void SetVolumeSW(uint) override {}
  void SetMuteSW(bool) override {}
  qint64 position_nanosec() const override {return 0;}
  qint64 length_nanosec() const override {return -1;}
  OutputDetailsList GetOutputsList() const override {return {};}
  bool ValidOutput(const QString&) override {return true;}
  QString DefaultOutput() const override {return {};}
  bool CustomDeviceSupport(const QString&) const override {return false;}
  bool ALSADeviceSupport(const QString&) const override {return false;}
  bool ExclusiveModeSupport(const QString&) const override {return false;}
  bool FadingSupport(const QString&) const override {return false;}
  void fail(){Q_EMIT Error(QStringLiteral("Internal data stream error"));Q_EMIT Error(QStringLiteral("debug"));state_=State::Error;Q_EMIT StateChanged(state_);Q_EMIT InvalidSongRequested(media_url());}
};

class RadioRecoveryTest : public QObject {
 Q_OBJECT
 SharedPtr<RecoveryManager> manager;
 SharedPtr<RecoveryEngine> engine;
 std::unique_ptr<Player> player;
 void select(const QUrl &url,bool radio=true){
   Song song;song.set_url(url);song.set_valid(true);song.set_source(radio?Song::Source::Stream:Song::Source::LocalFile);
   manager->list.InsertItems({PlaylistItemPtr(new SongPlaylistItem(song))});
   player->PlayAt(manager->list.rowCount()-1,false,0,EngineBase::TrackChangeType::First,Playlist::AutoScroll::Never,false);
 }
 void fireRetry(){player->radio_retry_timer_->stop();player->RetryRadio();}
 private Q_SLOTS:
 void init(){
   manager=make_shared<RecoveryManager>();engine=make_shared<RecoveryEngine>();
   player=std::make_unique<Player>(make_shared<TaskManager>(),make_shared<UrlHandlers>(),manager);
   player->engine_=engine;player->ConnectEngineSignals();select(QUrl(QStringLiteral("https://example.test/live")));
 }
 void cleanup(){player->Stop();player.reset();engine.reset();manager.reset();}
 void errorsAreQuietAndRetrySameRow(){
   QSignalSpy errors(player.get(),&Player::Error);engine->fail();
   QCOMPARE(errors.count(),0);QVERIFY(player->radio_retry_timer_->isActive());QCOMPARE(player->radio_retry_timer_->interval(),2000);
   QCOMPARE(manager->list.rowCount(),1);fireRetry();QCOMPARE(engine->plays,2);QCOMPARE(engine->offset,quint64(0));QCOMPARE(manager->list.current_row(),0);
 }
 void backoffAndStableReset(){
   for(int delay:{2000,5000,10000,20000,30000,60000,60000}){engine->fail();QCOMPARE(player->radio_retry_timer_->interval(),delay);fireRetry();}
   player->TrackPositionChanged(0);player->TrackPositionChanged(6);QVERIFY(!player->property("nasaqRadioReconnecting").toBool());
   engine->fail();QCOMPARE(player->radio_retry_timer_->interval(),2000);
 }
 void stopAndPauseCancel(){
   engine->fail();player->Pause();fireRetry();QCOMPARE(engine->plays,1);
   select(QUrl(QStringLiteral("https://example.test/other")));engine->fail();const int plays=engine->plays;player->Stop();fireRetry();QCOMPARE(engine->plays,plays);
 }
 void blockedStatesCancel_data(){QTest::addColumn<QByteArray>("name");for(const auto *name:{"camSuspended","camLicenseDenied","camStartupPrayerHoldUntil"})QTest::newRow(name)<<QByteArray(name);}
 void blockedStatesCancel(){QFETCH(QByteArray,name);engine->fail();
   player->setProperty(name.constData(),name=="camStartupPrayerHoldUntil"?QVariant(QDateTime::currentDateTimeUtc().addSecs(60)):QVariant(true));
   QVERIFY(!player->radio_retry_timer_->isActive());fireRetry();engine->fail();QCOMPARE(engine->plays,1);QVERIFY(!player->radio_retry_timer_->isActive());
   player->setProperty(name.constData(),QVariant());fireRetry();QCOMPARE(engine->plays,1); // Only CAM/user may resume.
   player->PlayAt(0,false,0,EngineBase::TrackChangeType::First,Playlist::AutoScroll::Never,false);QCOMPARE(engine->plays,2);
 }
 void changedStationAndRemovedRowCancel(){
   engine->fail();select(QUrl(QStringLiteral("https://example.test/new")));QVERIFY(!player->radio_retry_timer_->isActive());
   engine->fail();manager->list.Clear();fireRetry();QCOMPARE(engine->plays,2);
 }
 void eofRetriesInsteadOfSkipping(){Q_EMIT engine->TrackEnded();QVERIFY(player->radio_retry_timer_->isActive());QCOMPARE(manager->list.current_row(),0);}
 void fatalRadioQuietButFileErrorPreserved(){
   QSignalSpy errors(player.get(),&Player::Error);Q_EMIT engine->FatalError();QVERIFY(player->radio_retry_timer_->isActive());
   select(QUrl::fromLocalFile(QStringLiteral("C:/missing.wav")),false);Q_EMIT engine->Error(QStringLiteral("file failure"));QCOMPARE(errors.count(),1);QVERIFY(!player->radio_retry_timer_->isActive());
 }
 void lateUrlResultCannotPlayDuringHold(){
   player->setProperty("camSuspended",true);
   auto result=UrlHandler::LoadResult(currentUrl(),UrlHandler::LoadResult::Type::TrackAvailable);
   result.stream_url_=currentUrl();player->HandleLoadResult(result);QCOMPARE(engine->plays,1);
 }
 void realHttpOutageRecoveryAndAdhan(){
   // Real HTTP + production GStreamer/Player path; fakesink guarantees silent testing.
   Settings settings;settings.beginGroup(QStringLiteral("Backend"));settings.setValue(QStringLiteral("output"),QStringLiteral("fakesink"));settings.endGroup();
   player->Stop();player.reset();
   player=std::make_unique<Player>(make_shared<TaskManager>(),make_shared<UrlHandlers>(),manager);
   QVERIFY(player->engine()->Init());player->ConnectEngineSignals();
   QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));
   bool offline=true;int requests=0;QList<QPointer<QTcpSocket>> sockets;
   connect(&server,&QTcpServer::newConnection,&server,[&]{
     auto *socket=server.nextPendingConnection();sockets.append(socket);
     connect(socket,&QTcpSocket::readyRead,&server,[&,socket]{
       const QByteArray request=socket->property("request").toByteArray()+socket->readAll();socket->setProperty("request",request);
       if(socket->property("handled").toBool()||!request.contains("\r\n\r\n"))return;
       socket->setProperty("handled",true);++requests;
       if(offline){socket->write("HTTP/1.1 503 Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");socket->disconnectFromHost();return;}
       QByteArray wav;QDataStream out(&wav,QIODevice::WriteOnly);out.setByteOrder(QDataStream::LittleEndian);
       out.writeRawData("RIFF",4);out<<quint32(36+32000*120);out.writeRawData("WAVEfmt ",8);out<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(16000)<<quint32(32000)<<quint16(2)<<quint16(16);out.writeRawData("data",4);out<<quint32(32000*120);
       socket->write("HTTP/1.1 200 OK\r\nContent-Type: audio/wav\r\nConnection: close\r\n\r\n");socket->write(wav);socket->write(QByteArray(32000,'\0'));
       auto *feed=new QTimer(socket);connect(feed,&QTimer::timeout,socket,[socket]{if(socket->state()==QAbstractSocket::ConnectedState)socket->write(QByteArray(3200,'\0'));});feed->start(100);
     });
   });
   QSignalSpy errors(player.get(),&Player::Error);
   select(QUrl(QStringLiteral("http://127.0.0.1:%1/live.wav").arg(server.serverPort())));
   QTRY_VERIFY_WITH_TIMEOUT(player->property("nasaqRadioReconnecting").toBool(),15000);
   offline=false;
   QTRY_VERIFY_WITH_TIMEOUT(player->engine()->position_nanosec()>1000000000,15000);
   auto gst=qobject_cast<GstEngine*>(&*player->engine());QVERIFY(gst);auto pipeline=gst->current_pipeline_;
   QTRY_COMPARE_WITH_TIMEOUT(pipeline->state(),GST_STATE_PLAYING,10000);
   // Reproduce a stale sub-100% queue flag after playback has resumed.
   pipeline->buffering_=true;gst->BufferingStarted();QVERIFY(gst->property("nasaqBuffering").toBool());
   QTRY_VERIFY_WITH_TIMEOUT(!gst->property("nasaqBuffering").toBool(),7000);QVERIFY(!pipeline->is_buffering());
   // A real paused pipeline must never be declared recovered just by a flag.
   pipeline->SetState(GST_STATE_PAUSED);QTRY_COMPARE(pipeline->state(),GST_STATE_PAUSED);
   pipeline->buffering_=true;gst->BufferingStarted();pipeline->ConfirmPlaybackFlow();QVERIFY(pipeline->is_buffering());
   pipeline->buffering_=false;gst->BufferingFinished();pipeline->SetState(GST_STATE_PLAYING);QTRY_COMPARE(pipeline->state(),GST_STATE_PLAYING);
   QCOMPARE(errors.count(),0);const int row=manager->list.current_row();
   offline=true;for(const auto &socket:sockets)if(socket)socket->abort();
   QTRY_VERIFY_WITH_TIMEOUT(player->radio_retry_timer_->isActive(),15000);
   player->setProperty("camSuspended",true);player->engine()->SetBusinessGain(0);player->engine()->Stop(true);
   const int heldRequests=requests;QTest::qWait(2300);QCOMPARE(requests,heldRequests);QVERIFY(!player->radio_retry_timer_->isActive());
   offline=false;player->setProperty("camSuspended",false);fireRetry();QCOMPARE(requests,heldRequests);
   player->PlayAt(row,false,0,EngineBase::TrackChangeType::First,Playlist::AutoScroll::Never,false);
   QTRY_VERIFY_WITH_TIMEOUT(player->engine()->position_nanosec()>1000000000,15000);QCOMPARE(errors.count(),0);
   player->Stop();const int stoppedRequests=requests;QTest::qWait(2300);QCOMPARE(requests,stoppedRequests);
 }
 private:
 QUrl currentUrl() const {return player->current_item_->OriginalUrl();}
};

int main(int argc,char **argv){QApplication app(argc,argv);gst_init(nullptr,nullptr);QTimer glibEvents;QObject::connect(&glibEvents,&QTimer::timeout,[]{while(g_main_context_iteration(nullptr,false)){};});glibEvents.start(10);app.setOrganizationName(QStringLiteral("NASAQ-Recovery-Test"));app.setApplicationName(QStringLiteral("Radio"));QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());Q_INIT_RESOURCE(data);RadioRecoveryTest test;return QTest::qExec(&test,argc,argv);}
#include "radiorecovery_test.moc"
