// SPDX-License-Identifier: GPL-3.0-or-later
#undef QT_NO_CAST_FROM_ASCII
#include <QtTest>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QDockWidget>
#include <QScreen>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include "core/song.h"
#include "core/enginemetadata.h"
#include "engine/gstenginepipeline.h"
#include "nasaqtheme.h"
#include "audience.h"
#include "audio.h"
#include "scheduler.h"
#include "engine/gstengine.h"
#include "core/taskmanager.h"
#include "core/settings.h"

class VideoTransport final:public MusicTransport {
public:
 explicit VideoTransport(GstEngine &engine):engine(engine){}
 GstEngine &engine;bool held=false;int resumes=0;
 bool replace(const QStringList&)override{return false;}
 bool playing()const override{return engine.state()==EngineBase::State::Playing;}
 void suspend(bool on)override{held=on;engine.SetBusinessGain(on?0:1);if(on)engine.Pause();}
 void resume()override{if(!held){++resumes;engine.Unpause();}}
 void stop()override{engine.Stop(true);}
};
class VideoTest:public QObject {
 Q_OBJECT
private Q_SLOTS:
 void initTestCase(){
  QJsonArray screens;for(auto s:QGuiApplication::screens())screens.append(QJsonObject{{QStringLiteral("name"),s->name()},{QStringLiteral("width"),s->size().width()},{QStringLiteral("height"),s->size().height()}});
  auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty()){QFile f(dir+"/screens.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(screens).toJson());}
 }
 void audienceHasNoControlsAndPrayerReplacesFrame(){
  AudienceSurface surface;surface.resize(640,360);surface.show();QImage red(640,360,QImage::Format_RGB32);red.fill(Qt::red);QImage green(640,360,QImage::Format_RGB32);green.fill(Qt::green);
  surface.present(red,false,false,false,{},green);QCOMPARE(surface.grab().toImage().pixelColor(320,180),QColor(Qt::red));
  surface.present(red,true,false,false,{},green);QCOMPARE(surface.grab().toImage().pixelColor(320,180),QColor(Qt::green));QVERIFY(surface.prayerVisible());
  QTest::mouseMove(&surface,{50,50});QTest::mouseClick(&surface,Qt::RightButton);QVERIFY(surface.findChildren<QAbstractButton*>().isEmpty());QCOMPARE(surface.cursor().shape(),Qt::BlankCursor);QCOMPARE(surface.focusPolicy(),Qt::NoFocus);
  surface.present(red,false,true,false,green,{});QCOMPARE(surface.grab().toImage().pixelColor(320,180),QColor(Qt::green));
 }
 void defaultPrayerCardAndFit(){
  AudienceSurface surface;surface.resize(960,540);surface.show();QImage square(100,100,QImage::Format_RGB32);square.fill(Qt::red);surface.present(square,false,false,false,{},{});
  QCOMPARE(surface.grab().toImage().pixelColor(0,0),QColor("#102f28"));surface.present(square,false,false,true,{},{});QCOMPARE(surface.grab().toImage().pixelColor(0,0),QColor(Qt::red));
  surface.present(square,true,false,false,{},{});auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/prayer-card.png"));
 }
 void floatingPrayerDefaultCustomAndResume(){
  AudienceSurface surface;surface.enableVideoControls();surface.resize(1280,720);surface.show();surface.setPlayback(false,false,false,"","استئناف التشغيل / Resuming in  04:32",{});surface.present({},true,true,false,{},{});
  QVERIFY(surface.prayerVisible());QVERIFY(!surface.findChild<QWidget*>("videoControls")->isVisible());
  const auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/adhan-default-preview.png"));
  QImage custom(1280,720,QImage::Format_RGB32);custom.fill(Qt::blue);surface.present({},true,true,false,{},custom);QCOMPARE(surface.grab().toImage().pixelColor(640,360),QColor(Qt::blue));
  surface.present({},false,false,false,{},{});QVERIFY(!surface.prayerVisible());QVERIFY(surface.grab().toImage().pixelColor(640,360)!=QColor(Qt::blue));
 }
 void ownedImageSurvivesOriginalRemoval(){
  QTemporaryDir dir;QImage image(200,100,QImage::Format_RGB32);image.fill(Qt::blue);auto original=dir.filePath("chosen.png");QVERIFY(image.save(original));auto owned=AudienceDisplay::importImage(original,dir.filePath("owned"));QVERIFY(QFile::remove(original));QVERIFY(!QImage(owned).isNull());
  QFile invalid(dir.filePath("bad.png"));QVERIFY(invalid.open(QIODevice::WriteOnly));invalid.write("invalid");invalid.close();QVERIFY_THROWS_EXCEPTION(std::runtime_error,AudienceDisplay::importImage(invalid.fileName(),dir.filePath("owned")));
 }
 void missingMonitorNeverFallsBackToOperator(){
  Store store(":memory:");store.set("audience_restore","1");store.set("audience_screen","missing-monitor");QMainWindow main;main.show();AudienceDisplay display(store,&main);QTest::qWait(100);QVERIFY(!display.externalActive());
  QImage frame(100,100,QImage::Format_RGB32);frame.fill(Qt::blue);display.updateFrame(frame,1,false,false);main.showMinimized();display.updateFrame(frame,1,true,true);QVERIFY(display.preview()->prayerVisible());QVERIFY(!display.externalActive());display.stopExternal();QCOMPARE(store.get("audience_restore"),QString("0"));
 }
 void externalScreenRemainsCleanWhenOperatorMinimized(){
  if(QGuiApplication::screens().size()<2)QSKIP("A second physical display in Extend mode is required");
  QMainWindow main;main.show();QScreen *external=nullptr;for(auto s:QGuiApplication::screens())if(s!=main.screen()){external=s;break;}QVERIFY(external);
  Store store(":memory:");store.set("audience_screen",AudienceDisplay::screenKey(external));store.set("audience_restore","1");AudienceDisplay display(store,&main);
  QTRY_VERIFY(display.externalActive());QWidget *surface=nullptr;for(auto w:QApplication::topLevelWidgets())if(w->windowTitle()=="NASAQ Audience")surface=w;QVERIFY(surface);QVERIFY(surface->isFullScreen());QVERIFY(surface->windowFlags()&Qt::WindowDoesNotAcceptFocus);QVERIFY(!surface->parentWidget());
  QImage frame(640,360,QImage::Format_RGB32);frame.fill(Qt::blue);display.updateFrame(frame,1,false,false);main.showMinimized();QTest::qWait(200);QVERIFY(surface->isVisible());QVERIFY(display.externalActive());QTest::mouseMove(surface,{20,20});QVERIFY(surface->findChildren<QAbstractButton*>().isEmpty());
  display.updateFrame({},1,true,true);QVERIFY(display.preview()->prayerVisible());auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface->grab().save(dir+"/external-prayer.png"));display.stopExternal();QVERIFY(!surface->isVisible());
 }
 void settingsRespectThemeAndCancelKeepsPreferences(){
  qApp->setProperty("nasaqLiveLanguage",QStringLiteral("ar"));ApplyNasaqTheme();Store store(":memory:");store.set("audience_crop","1");QMainWindow main;main.show();AudienceDisplay display(store,&main);bool seen=false;
  QTimer timer;connect(&timer,&QTimer::timeout,&main,[&]{if(auto dialog=main.findChild<QDialog*>("audienceSettings")){seen=true;QVERIFY(dialog->findChild<QComboBox*>("audienceScreenSelector"));auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(dialog->grab().save(dir+"/settings-ar.png"));dialog->reject();}});timer.start(100);display.settings();QVERIFY(seen);QCOMPARE(store.get("audience_crop"),QString("1"));QVERIFY(store.get("audience_screen").isEmpty());
 }
 void saveDisplaySettingsWithoutScreenOrImages(){
  QTemporaryDir dir;const auto database=dir.filePath("display.sqlite");
  {Store store(database);QMainWindow main;main.show();AudienceDisplay display(store,&main);
   for(int attempt=0;attempt<2;++attempt){bool clicked=false;
    QTimer::singleShot(100,&main,[&]{auto dialog=main.findChild<QDialog*>("audienceSettings");if(!dialog)return;clicked=true;dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();});
    display.settings();QVERIFY(clicked);QVERIFY(!display.externalActive());QVERIFY(!display.preview()->isVisible());
   }
   auto rows=store.query("SELECT COUNT(*) FROM settings WHERE key LIKE 'audience_%' AND value IS NOT NULL");QVERIFY(rows.next());QCOMPARE(rows.value(0).toInt(),5);
  }
  Store reopened(database);QCOMPARE(reopened.get("audience_crop"),QString("0"));QCOMPARE(reopened.get("audience_restore"),QString("0"));QVERIFY(reopened.get("audience_prayer_image").isEmpty());
 }
 void failedDisplaySaveRollsBackAndKeepsDialogOpen(){
  Store store(":memory:");store.set("audience_crop","0");
  store.exec("CREATE TRIGGER reject_display_save BEFORE INSERT ON settings WHEN NEW.key='audience_prayer_image' BEGIN SELECT RAISE(ABORT,'simulated storage error'); END");
  QMainWindow main;main.show();AudienceDisplay display(store,&main);bool remained=false,feedback=false;
  QTimer::singleShot(100,&main,[&]{auto dialog=main.findChild<QDialog*>("audienceSettings");if(!dialog)return;
   for(auto check:dialog->findChildren<QCheckBox*>())if(check->text().contains(QString::fromUtf8("القص"))||check->text().contains("cropping"))check->setChecked(true);
   dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();remained=dialog->isVisible();feedback=!dialog->findChild<QLabel*>("audienceSaveFeedback")->text().isEmpty();dialog->reject();
  });display.settings();QVERIFY(remained);QVERIFY(feedback);QCOMPARE(store.get("audience_crop"),QString("0"));auto row=store.query("SELECT COUNT(*) FROM settings WHERE key='audience_screen'");QVERIFY(row.next());QCOMPARE(row.value(0).toInt(),0);QVERIFY(!display.externalActive());
 }
 void floatingPlayerDoesNotResizeMainOrRestartWhenHidden(){
  Store store(":memory:");QMainWindow main;main.resize(700,500);main.show();AudienceDisplay display(store,&main);
  const auto before=main.size();display.updateFrame({},-1,false,false);display.setPlayback(true,true,false,"Teletext Radio","Pause in 00:12:00",QVector<float>(24,.3f));
  auto window=display.preview()->window();QVERIFY(window!=&main);QVERIFY(window->isVisible());QCOMPARE(main.size(),before);QVERIFY(main.findChildren<QDockWidget*>().isEmpty());
  window->close();QVERIFY(!window->isVisible());display.setPlayback(true,true,false,"Teletext Radio","Pause in 00:11:59",{});QVERIFY(!window->isVisible());
  display.showFloating();QVERIFY(window->isVisible());QVERIFY(window->windowFlags()&Qt::WindowMinimizeButtonHint);
 }
 void vinylRotationStopsForPauseReconnectAndPrayer(){
  AudienceSurface surface;surface.resize(1100,720);surface.show();surface.present({},false,false,false,{},{});
  surface.setPlayback(true,true,false,"Teletext Radio",QString::fromUtf8("متبقي للتوقف: 00:12:00"),QVector<float>(24,.5f));QTest::qWait(50);
  surface.setPlayback(true,true,false,"Teletext Radio",QString::fromUtf8("متبقي للتوقف: 00:12:00"),QVector<float>(24,.5f));QVERIFY(surface.rotation()>0);
  const auto angle=surface.rotation();QTest::qWait(50);surface.setPlayback(false,true,false,"Teletext Radio","",{});QCOMPARE(surface.rotation(),angle);
  surface.setPlayback(true,true,true,"Teletext Radio","",{});QCOMPARE(surface.rotation(),angle);
  surface.present({},true,true,false,{},{});surface.setPlayback(true,true,false,"Teletext Radio","",{});QCOMPARE(surface.rotation(),angle);
  surface.present({},false,false,false,{},{});surface.setPlayback(true,true,false,"Teletext Radio",QString::fromUtf8("متبقي للتوقف: 00:12:00"),QVector<float>{.1f,.2f,.4f,.6f,.8f,.7f,.5f,.3f,.6f,.8f,.9f,.6f,.5f,.3f,.1f,.2f,.4f,.7f,.6f,.3f,.2f,.1f,.3f,.2f});
  const auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/floating-vinyl.png"));
  surface.resize(480,320);if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/floating-compact.png"));
 }
 void albumSleeveClearsAndRejectsStaleOrFallbackCovers(){
  Store store(":memory:");QMainWindow main;AudienceDisplay display(store,&main);
  QImage cover(300,300,QImage::Format_RGB32);cover.fill(QColor("#577b76"));
  {QPainter p(&cover);p.setPen(QColor("#fff6da"));QFont f=p.font();f.setPixelSize(34);p.setFont(f);p.drawText(cover.rect(),Qt::AlignCenter,"ALBUM\nPREVIEW");}
  display.beginTrack("first");display.setAlbumCover("first",cover,true);QVERIFY(display.preview()->hasAlbumCover());
  display.updateFrame({},-1,false,false);display.setPlayback(true,false,false,"Album preview","",QVector<float>(24,.45f));
  const auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");
  display.preview()->window()->resize(1100,720);QTest::qWait(50);
  if(!dir.isEmpty())QVERIFY(display.preview()->grab().save(dir+"/album-sleeve.png"));
  display.preview()->window()->resize(480,320);QTest::qWait(50);
  if(!dir.isEmpty())QVERIFY(display.preview()->grab().save(dir+"/album-sleeve-compact.png"));
  display.beginTrack("second");QVERIFY(!display.preview()->hasAlbumCover());
  display.setAlbumCover("first",cover,true);QVERIFY(!display.preview()->hasAlbumCover());
  display.setAlbumCover("second",cover,false);QVERIFY(!display.preview()->hasAlbumCover());
  display.setAlbumCover("second",cover,true);QVERIFY(display.preview()->hasAlbumCover());
  display.beginTrack({});QVERIFY(!display.preview()->hasAlbumCover());
  display.setAlbumCover("second",cover,true);QVERIFY(!display.preview()->hasAlbumCover());
 }
 void prayerCaptionRespectsDisabledPauseAndEarlyLead(){
  qApp->setProperty("nasaqLiveLanguage",QStringLiteral("en"));
  Store store(":memory:");const auto now=QDateTime::fromString("2026-10-26T08:00:00Z",Qt::ISODate);
  QVERIFY(AudienceDisplay::prayerCaption(store,now).isEmpty());
  store.add({"test-prayer","prayer",{},now.addSecs(600),60});store.set("prayer_lead_minutes","2");
  QCOMPARE(AudienceDisplay::prayerCaption(store,now),QString("Pause in: 00:08:00"));
  store.set("prayer_hold_until",now.addSecs(120).toString(Qt::ISODate));
  QVERIFY(AudienceDisplay::prayerCaption(store,now).startsWith("استئناف التشغيل / Resuming in  "));
  store.set("prayer_pause_enabled","0");
  QCOMPARE(AudienceDisplay::prayerCaption(store,now),QString("Next adhan: ")+now.addSecs(600).toLocalTime().toString("HH:mm"));
  store.exec("DELETE FROM events");QVERIFY(AudienceDisplay::prayerCaption(store,now).isEmpty());
 }
 void photographicLayersAreBundled(){
  for(const auto name:{"base","record","arm"}){QImage asset(QString(":/nasaq/turntable-%1.png").arg(name));QVERIFY(!asset.isNull());QVERIFY(asset.width()>=1000);}
  const QImage record(":/nasaq/turntable-record.png"),arm(":/nasaq/turntable-arm.png");
  QCOMPARE(record.pixelColor(0,0).alpha(),0);QCOMPARE(arm.pixelColor(0,0).alpha(),0);
 }
 void videoFilenameWinsOverSoundtrackTags(){
  Song song;song.set_url(QUrl::fromLocalFile("C:/media/Training-lesson.MP4"));song.set_title(QStringLiteral("Soundtrack title"));song.set_artist(QStringLiteral("Composer"));
  EngineMetadata metadata;metadata.title="Another audio title";metadata.artist="Another composer";song.MergeFromEngineMetadata(metadata);
  QCOMPARE(song.PrettyTitle(),QString("Training-lesson.MP4"));QCOMPARE(song.PrettyTitleWithArtist(),song.PrettyTitle());QCOMPARE(song.TitleWithCompilationArtist(),song.PrettyTitle());
  song.set_url(QUrl::fromLocalFile("C:/media/song.m4a"));QCOMPARE(song.PrettyTitle(),QString("Soundtrack title"));QVERIFY(song.PrettyTitleWithArtist().contains("Composer"));
 }
 void manualVideoReopensHiddenWindowButAutomaticResumeDoesNot(){
  Store store(":memory:");QMainWindow main;AudienceDisplay display(store,&main);display.showFloating();auto window=display.preview()->window();
  window->close();QVERIFY(!window->isVisible());display.updateFrame({},42,false,false);display.setPlayback(true,false,false,"video","",{});QVERIFY(!window->isVisible());
  display.manualMediaRequested(QUrl::fromLocalFile("C:/media/video.mp4"));QVERIFY(window->isVisible());
  window->close();display.manualMediaRequested(QUrl::fromLocalFile("C:/media/audio.mp3"));QVERIFY(!window->isVisible());
 }
 void spaceWorksInBothOperatorWindowsButNotTextFields(){
  Store store(":memory:");QMainWindow main;main.show();AudienceDisplay display(store,&main);QSignalSpy toggles(&display,&AudienceDisplay::playPauseRequested);
  QTest::keyClick(&main,Qt::Key_Space);QCOMPARE(toggles.count(),1);
  display.showFloating();QTest::keyClick(display.preview()->window(),Qt::Key_Space);QCOMPARE(toggles.count(),2);
  QKeyEvent repeat(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier," ",true,1);QApplication::sendEvent(display.preview()->window(),&repeat);QCOMPARE(toggles.count(),2);
  QLineEdit edit(&main);edit.show();main.activateWindow();edit.setFocus();QTRY_VERIFY(edit.hasFocus());QTest::keyClick(&edit,Qt::Key_Space);QCOMPARE(edit.text(),QString(" "));QCOMPARE(toggles.count(),2);
 }
 void videoControlsHideAfterThreeSecondsAndSeekWithoutPrayerOverlay(){
  AudienceSurface surface;surface.enableVideoControls();surface.resize(960,540);surface.show();
  QImage frame(960,540,QImage::Format_RGB32);frame.fill(QColor("#29483e"));surface.present(frame,false,false,false,{},{});
  surface.setPlayback(true,false,false,"Video","PRAYER COUNTDOWN",QVector<float>(24,.7f));surface.setTimeline(65,600);surface.revealControls();
  auto controls=surface.findChild<QWidget*>("videoControls");auto seek=surface.findChild<QSlider*>("videoSeek");QVERIFY(controls->isVisible());QCOMPARE(seek->value(),1083);
  QSignalSpy play(&surface,&AudienceSurface::playPauseRequested),stop(&surface,&AudienceSurface::stopRequested),previous(&surface,&AudienceSurface::previousRequested),next(&surface,&AudienceSurface::nextRequested),full(&surface,&AudienceSurface::fullscreenRequested),seeks(&surface,&AudienceSurface::seekRequested);
  for(const auto name:{"play","stop","previous","next","full"})QTest::mouseClick(surface.findChild<QToolButton*>(QString("video_%1").arg(name)),Qt::LeftButton);
  QCOMPARE(play.count(),1);QCOMPARE(stop.count(),1);QCOMPARE(previous.count(),1);QCOMPARE(next.count(),1);QCOMPARE(full.count(),1);
  seek->setSliderDown(true);seek->setValue(5000);surface.setTimeline(66,600);QCOMPARE(seek->value(),5000);seek->setSliderDown(false);QCOMPARE(seeks.count(),1);QCOMPARE(seeks.first().first().toULongLong(),quint64(300));
  const auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/video-controls.png"));
  QTest::qWait(2600);QVERIFY(controls->isVisible());QTRY_VERIFY_WITH_TIMEOUT(!controls->isVisible(),1000);
  const auto image=surface.grab().toImage();QCOMPARE(image.pixelColor(image.width()-80,image.height()-40),QColor("#29483e"));
  if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/video-controls-hidden.png"));
  QMouseEvent movement(QEvent::MouseMove,QPointF(35,35),QPointF(surface.mapToGlobal(QPoint(35,35))),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(&surface,&movement);QVERIFY(!controls->isVisible());
  const QPoint near(controls->geometry().center());QMouseEvent approach(QEvent::MouseMove,QPointF(near),QPointF(surface.mapToGlobal(near)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(&surface,&approach);QVERIFY(controls->isVisible());
  QTest::mouseClick(seek,Qt::LeftButton,Qt::NoModifier,QPoint(seek->width()*3/4,seek->height()/2));QCOMPARE(seeks.count(),2);QVERIFY(seeks.last().first().toULongLong()>440);QVERIFY(seeks.last().first().toULongLong()<465);
  surface.resize(480,320);if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/video-controls-compact.png"));
  surface.present(frame,true,true,false,{},{});QVERIFY(!controls->isVisible());
  AudienceSurface external;QVERIFY(external.findChildren<QToolButton*>().isEmpty());
 }
 void videoControlFullscreenRestoresWithEscape(){
  Store store(":memory:");QMainWindow main;AudienceDisplay display(store,&main);QImage frame(960,540,QImage::Format_RGB32);frame.fill(Qt::darkGreen);display.updateFrame(frame,1,false,false);
  auto window=display.preview()->window();auto full=display.preview()->findChild<QToolButton*>("video_full");QTest::mouseClick(full,Qt::LeftButton);QVERIFY(window->isFullScreen());QTest::keyClick(window,Qt::Key_Escape);QVERIFY(!window->isFullScreen());
 }
 void audioControlsAdaptToRadioAndFadeWithoutInterruptingInteraction(){
  AudienceSurface surface;surface.enableVideoControls();surface.resize(960,640);surface.show();surface.present({},false,false,false,{},{});
  surface.setPlayback(true,false,false,"Song","",{});surface.setTimeline(60,240);surface.revealControls();
  auto controls=surface.findChild<QWidget*>("videoControls");auto seek=surface.findChild<QSlider*>("videoSeek");auto volume=surface.findChild<QSlider*>("mediaVolume");
  QVERIFY(controls->isVisible());QVERIFY(seek->isVisible());QCOMPARE(seek->value(),2500);
  QSignalSpy volumeChanges(&surface,&AudienceSurface::volumeRequested),seeks(&surface,&AudienceSurface::seekRequested);
  surface.setVolume(64);QCOMPARE(volume->value(),64);QCOMPARE(volumeChanges.count(),0);volume->setValue(30);QCOMPARE(volumeChanges.count(),1);
  seek->setSliderDown(true);QTest::qWait(3500);QVERIFY(controls->isVisible());seek->setSliderDown(false);
  surface.setPlayback(true,true,false,"Radio","",{});surface.setTimeline(600,1200);QVERIFY(!seek->isVisible());QVERIFY(!seek->isEnabled());
  surface.setNavigation(false,false);QVERIFY(!surface.findChild<QToolButton*>("video_next")->isEnabled());
  const auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/radio-controls.png"));
  surface.revealControls();QTRY_VERIFY_WITH_TIMEOUT(!controls->isVisible(),4000);
  surface.revealControls();QVERIFY(controls->isVisible());volume->setFocus(Qt::TabFocusReason);QTest::keyClick(volume,Qt::Key_Right);QTest::qWait(3500);QVERIFY(controls->isVisible());volume->clearFocus();
  surface.setPlayback(false,false,false,"Song","",{});surface.setTimeline(60,240);surface.setNavigation(true,true);surface.revealControls();
  if(!dir.isEmpty())QVERIFY(surface.grab().save(dir+"/song-controls.png"));
  surface.present({},true,true,false,{},{});QVERIFY(!controls->isVisible());
 }
 void tonearmTracksSongProgressAndStaysCenteredForRadio(){
  AudienceSurface surface;surface.present({},false,false,false,{},{});surface.setPlayback(true,false,false,"Song","",{});
  surface.setTimeline(0,300);QCOMPARE(surface.tonearmAngle(),-22.);surface.setTimeline(150,300);QCOMPARE(surface.tonearmAngle(),-12.);surface.setTimeline(300,300);QCOMPARE(surface.tonearmAngle(),-2.);
  surface.setPlayback(false,false,false,"Song","",{});QCOMPARE(surface.tonearmAngle(),-2.);
  surface.setTimeline(75,300);QCOMPARE(surface.tonearmAngle(),-17.);
  surface.setPlayback(true,true,false,"Radio","",{});surface.setTimeline(0,0);QCOMPARE(surface.tonearmAngle(),-12.);surface.setTimeline(5000,1);QCOMPARE(surface.tonearmAngle(),-12.);
  surface.setPlayback(true,true,true,"Radio","",{});QCOMPARE(surface.tonearmAngle(),-12.);
  surface.present({},true,true,false,{},{});QCOMPARE(surface.tonearmAngle(),-48.);
 }
 void actualVideoPlaybackPauseSeekAndEarlyPrayer(){
  const QString file=qEnvironmentVariable("NASAQ_VIDEO_FIXTURE");QVERIFY2(QFileInfo::exists(file),"MP4 fixture required");
  Settings settings;settings.beginGroup(QStringLiteral("Backend"));settings.setValue(QStringLiteral("output"),QStringLiteral("fakesink"));settings.setValue(QStringLiteral("fadeout_pause_enabled"),false);settings.endGroup();
  GstEngine engine(std::make_shared<TaskManager>());QVERIFY(engine.Init());QSignalSpy errors(&engine,&EngineBase::Error);const auto url=QUrl::fromLocalFile(file);
  QVERIFY(engine.Load(url,url,EngineBase::TrackChangeType::First,false,0,0,std::nullopt));QVERIFY(engine.Play(false,0));
  QImage frame;QTRY_VERIFY_WITH_TIMEOUT(([&]{if(frame.isNull())frame=engine.TakeVideoFrame();return !frame.isNull();}()),15000);QVERIFY(frame.width()>=640);QVERIFY(engine.VideoGeneration()>=0);
  auto dir=qEnvironmentVariable("NASAQ_VIDEO_EVIDENCE");if(!dir.isEmpty())QVERIFY(frame.save(dir+"/decoded-video.png"));
  QTRY_VERIFY_WITH_TIMEOUT(engine.position_nanosec()>1000000000,10000);engine.Seek(3000000000);QTRY_VERIFY_WITH_TIMEOUT(engine.position_nanosec()>=3000000000,10000);
  // Reproduce a temporary buffering pause and ensure the user's pause survives
  // the subsequent buffer-complete message instead of resuming unexpectedly.
  const auto pipeline=engine.current_pipeline_;
  auto buffering=gst_message_new_buffering(GST_OBJECT(pipeline->audioqueue_),31);pipeline->BufferingMessageReceived(buffering);gst_message_unref(buffering);
  QVERIFY(pipeline->is_buffering());QCOMPARE(engine.state(),EngineBase::State::Playing);
  engine.Pause();QCOMPARE(engine.state(),EngineBase::State::Paused);
  buffering=gst_message_new_buffering(GST_OBJECT(pipeline->audioqueue_),100);pipeline->BufferingMessageReceived(buffering);gst_message_unref(buffering);
  QTest::qWait(200);QCOMPARE(engine.state(),EngineBase::State::Paused);engine.Unpause();QTRY_COMPARE(engine.state(),EngineBase::State::Playing);
  Store store(":memory:");store.set("prayer_lead_minutes","1");store.set("prayer_pause_minutes","1");VideoTransport transport(engine);Audio audio(store,nullptr,&transport);Scheduler scheduler(store);const auto now=QDateTime::currentDateTimeUtc();const auto at=now.addSecs(60);store.add({QStringLiteral("prayer"),QStringLiteral("prayer"),QString(),at,60});
  QTRY_COMPARE(engine.state(),EngineBase::State::Playing);scheduler.tick(now,[&](const Event &e){return audio.dispatch(e);});QVERIFY(audio.prayerActive());QVERIFY(transport.held);QTRY_COMPARE(engine.state(),EngineBase::State::Paused);auto position=engine.position_nanosec();QTest::qWait(350);QVERIFY(qAbs(engine.position_nanosec()-position)<100000000);
  AudienceSurface surface;surface.present(frame,audio.prayerActive(),true,false,{},{});QVERIFY(surface.prayerVisible());audio.tick(at.addSecs(59));QVERIFY(transport.held);audio.tick(at.addSecs(60));QVERIFY(!transport.held);QTRY_COMPARE(engine.state(),EngineBase::State::Playing);QCOMPARE(transport.resumes,1);QCOMPARE(errors.count(),0);
  engine.Stop(true);QCOMPARE(engine.VideoGeneration(),-1);QVERIFY(engine.TakeVideoFrame().isNull());
 }
};
int main(int argc,char **argv){qputenv("QT_MEDIA_BACKEND","windows");QApplication app(argc,argv);gst_init(nullptr,nullptr);QTimer glib;QObject::connect(&glib,&QTimer::timeout,[]{while(g_main_context_iteration(nullptr,false)){};});glib.start(10);app.setOrganizationName("NASAQ-Video-Test");app.setApplicationName("Video");QTemporaryDir temp;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());Q_INIT_RESOURCE(data);VideoTest test;return QTest::qExec(&test,argc,argv);}
#include "video_test.moc"







