// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QtWidgets>
#include "window.h"
#include "prayers.h"
#include "storeupdates.h"
#include "nasaqlanguage.h"
#include "livelanguage.h"
class ReviewTest:public QObject {
 Q_OBJECT
 QByteArray calendar(QString offset="+04:00"){
 QJsonObject timings;for(const auto &n:QStringList{"Fajr","Dhuhr","Asr","Maghrib","Isha"})timings[n]="2030-01-02T12:00:00"+offset;
 return QJsonDocument(QJsonObject{{"code",200},{"data",QJsonArray{QJsonObject{{"timings",timings}}}}}).toJson();
 }
private slots:
 void storeUpdateStates(){
  int calls=0;StoreUpdates panel(true,nullptr,[&calls](quintptr){++calls;if(calls==1)return StoreUpdateResult{true,{}};if(calls==2)return StoreUpdateResult{false,"offline"};return StoreUpdateResult{};});
  QSignalSpy available(&panel,&StoreUpdates::availabilityChanged);auto button=panel.findChild<QPushButton*>("checkStoreUpdates");QVERIFY(button);
  panel.check();panel.check();QVERIFY(!button->isEnabled());QTRY_COMPARE(available.count(),1);QCOMPARE(calls,1);QVERIFY(available.at(0).at(0).toBool());QVERIFY(button->isEnabled());
  panel.check();QTRY_VERIFY(button->isEnabled());QCOMPARE(available.count(),1);QCOMPARE(panel.findChild<QLabel*>("storeUpdateStatus")->toolTip(),QString("offline"));
  panel.check();QTRY_COMPARE(available.count(),2);QVERIFY(!available.at(1).at(0).toBool());
 }
 void unpackagedDoesNotClaimStoreUpdateStatus(){int calls=0;StoreUpdates panel(false,nullptr,[&calls](quintptr){++calls;return StoreUpdateResult{};});panel.check();QCOMPARE(calls,0);QVERIFY(!panel.findChild<QPushButton*>("checkStoreUpdates")->isEnabled());QVERIFY(panel.findChild<QPushButton*>("openStoreUpdate")->isEnabled());}

 void monthlyScheduleAlwaysEnabled_data(){QTest::addColumn<QString>("previous");QTest::newRow("first-use")<<QString();QTest::newRow("previously-disabled")<<QString("0");QTest::newRow("enabled")<<QString("1");}
 void monthlyScheduleAlwaysEnabled(){QFETCH(QString,previous);Store s(":memory:");if(!previous.isNull())s.set("prayer_enabled",previous);s.set("prayer_pause_enabled","0");PrayerService service(s);QCOMPARE(s.get("prayer_enabled"),QString("1"));QCOMPARE(s.get("prayer_pause_enabled"),QString("0"));QVERIFY(s.get("prayer_city").isEmpty());QVERIFY(!PrayerService::nextPause(s,QDateTime::currentDateTimeUtc()).isValid());}

 void firstUseWaitsForCity(){Store s(":memory:");PrayerService service(s);QSignalSpy completed(&service,&PrayerService::completed);QTest::qWait(5500);QVERIFY(completed.isEmpty());QVERIFY(s.get("prayer_city").isEmpty());QVERIFY(s.get("prayer_sync_date").isEmpty());}
 void adhanPopupOwnedByMainPlayer_data(){QTest::addColumn<bool>("minimized");QTest::newRow("visible-player")<<false;QTest::newRow("minimized-player")<<true;}
 void adhanPopupOwnedByMainPlayer(){
  QFETCH(bool,minimized);Store s(":memory:");s.set("prayer_city","Dubai");s.set("prayer_hold_until",QDateTime::currentDateTimeUtc().addSecs(8).toString(Qt::ISODate));
  QMainWindow main;main.resize(900,650);main.show();if(minimized)main.showMinimized();
  auto workspace=std::make_unique<Window>(s,nullptr,&main);workspace->hide();
  QTimer dismiss;connect(&dismiss,&QTimer::timeout,workspace.get(),[&]{if(auto d=workspace->findChild<QDialog*>("activationDialog"))d->reject();});dismiss.start(30);
  auto popup=main.findChild<QDialog*>("prayerPausePopup");QVERIFY(popup);QCOMPARE(popup->parentWidget(),&main);
  QTRY_VERIFY_WITH_TIMEOUT(popup->isVisible(),3000);QVERIFY(!workspace->isVisible());QVERIFY(main.isVisible());QVERIFY(!main.isMinimized());
  QCOMPARE(popup->windowModality(),Qt::WindowModal);QVERIFY(popup->windowHandle());QCOMPARE(popup->windowHandle()->transientParent(),main.windowHandle());
  QVERIFY(main.frameGeometry().contains(popup->frameGeometry().center()));
  auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())QVERIFY(popup->grab().save(dir+QString("/main-adhan-%1.png").arg(minimized)));
  popup->reject();QVERIFY(!popup->isVisible());QVERIFY(!s.get("prayer_hold_until").isEmpty());
  QPointer<QDialog> retained=popup;workspace.reset();QVERIFY(retained.isNull());
 }

 void adhanToggleAndLiveLanguage(){
  QSettings settings("NASAQ","Preferences");const auto saved=settings.value("language");const auto oldOverride=qApp->property("nasaqLiveLanguage");
  Store s(":memory:");Window w(s);w.show();w.openSection(3);
  auto toggle=w.findChild<QCheckBox*>("adhanAutoPause");auto language=w.findChild<QComboBox*>("interfaceLanguage");QVERIFY(toggle);QVERIFY(language);
  toggle->click();QCOMPARE(s.get("prayer_pause_enabled"),QString("0"));toggle->click();QCOMPARE(s.get("prayer_pause_enabled"),QString("1"));
  auto userText=new QLineEdit(QStringLiteral("Music"),&w);
  QSignalSpy changed(&w,&Window::interfaceLanguageChanged);
  for(const auto &code:QStringList{"en","ar","en","ar"}){
   language->setCurrentIndex(language->findData(code));Q_EMIT language->activated(language->currentIndex());
   QTest::qWait(50);
   auto nav=w.findChild<QListWidget*>("sectionNavigation");auto stack=w.findChild<QStackedWidget*>("workspaceSections");QCOMPARE(nav->x()>stack->x(),code=="ar");
   QCOMPARE(NasaqLanguage(),code);QCOMPARE(w.layoutDirection(),code=="ar"?Qt::RightToLeft:Qt::LeftToRight);
   QCOMPARE(toggle->text(),code=="ar"?QStringLiteral("إيقاف الموسيقى تلقائيًا عند الأذان"):QStringLiteral("Automatically pause music at adhan"));
   QCOMPARE(w.findChild<QListWidget*>("sectionNavigation")->item(3)->text(),code=="ar"?QStringLiteral("أوقات الأذان"):QStringLiteral("Adhan times"));
   QCOMPARE(userText->text(),QString("Music"));
   const auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())QVERIFY(w.grab().save(dir+"/live-adhan-"+code+".png"));
  }
  QCOMPARE(changed.count(),4);if(saved.isValid())settings.setValue("language",saved);else settings.remove("language");qApp->setProperty("nasaqLiveLanguage",oldOverride);qApp->setLayoutDirection(NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight);
 }

 void workspaceSectionsKeepTasksSeparate(){
  Store s(":memory:");Window w(s);w.show();
  auto nav=w.findChild<QListWidget*>("sectionNavigation");auto stack=w.findChild<QStackedWidget*>("workspaceSections");
  QVERIFY(nav);QVERIFY(stack);QCOMPARE(nav->count(),6);QVERIFY(w.findChild<QPushButton*>("defaultAudioPlayer"));
  auto mic=w.findChild<QPushButton*>("holdToTalk");auto license=w.findChild<QLabel*>("licenseStatus");auto table=w.findChild<QTableWidget*>();
  QVERIFY(mic);QVERIFY(license);QVERIFY(table);
  QVERIFY(stack->widget(0)->isAncestorOf(mic));QVERIFY(stack->widget(5)->isAncestorOf(license));QVERIFY(stack->widget(2)->isAncestorOf(table));
  QTimer dismiss;connect(&dismiss,&QTimer::timeout,&w,[&]{if(auto d=w.findChild<QDialog*>("activationDialog"))d->reject();});dismiss.start(30);
  QTest::qWait(900);
  for(int i=0;i<6;++i){w.openSection(i);QCOMPARE(stack->currentIndex(),i);QVERIFY(w.windowTitle().contains(Window::sectionNames()[i]));QTest::qWait(50);
   QCOMPARE(mic->isVisible(),i==0);QCOMPARE(license->isVisible(),i==5);QCOMPARE(table->isVisible(),i==2);
   auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())QVERIFY(w.grab().save(dir+QString("/section-%1.png").arg(i)));
  }
  w.openSection(-1);QCOMPARE(stack->currentIndex(),5);w.openSection(6);QCOMPARE(stack->currentIndex(),5);
  nav->setCurrentRow(0);QCOMPARE(stack->currentIndex(),0);
 }
 void prayerPopupShowsCityAndClosesOnExpiry(){
  Store s(":memory:");s.set("prayer_city","Dubai");s.set("prayer_hold_until",QDateTime::currentDateTimeUtc().addSecs(5).toString(Qt::ISODate));Window w(s);w.show();
  QTimer dismiss;connect(&dismiss,&QTimer::timeout,&w,[&]{if(auto d=w.findChild<QDialog*>("activationDialog"))d->reject();});dismiss.start(30);
  auto popup=w.findChild<QDialog*>("prayerPausePopup");QVERIFY(popup);QTRY_VERIFY_WITH_TIMEOUT(popup->isVisible(),3000);
  auto text=popup->findChild<QLabel*>();QVERIFY(text);QVERIFY(text->text().contains("Dubai")||text->text().contains(QStringLiteral("دبي")));
  auto skip=popup->findChild<QPushButton*>("skipPrayer");QVERIFY(skip);QVERIFY(!skip->isEnabled());
  auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())QVERIFY(popup->grab().save(dir+"/prayer-popup.png"));
  QTRY_VERIFY_WITH_TIMEOUT(!popup->isVisible(),7000);
 }
 void pauseReadinessRequiresActualFutureEvent(){Store s(":memory:");auto now=QDateTime::currentDateTimeUtc();s.set("prayer_pause_enabled","1");s.set("prayer_enabled","1");QVERIFY(!PrayerService::nextPause(s,now).isValid());s.add({"p","prayer","",now.addSecs(60),60});QCOMPARE(PrayerService::nextPause(s,now),now.addSecs(60));s.set("prayer_pause_enabled","0");QVERIFY(!PrayerService::nextPause(s,now).isValid());s.set("prayer_pause_enabled","1");s.finish("p","dispatched");QVERIFY(!PrayerService::nextPause(s,now).isValid());}

 void chimePreferencesNoSoundAndPersistence(){Store s(":memory:");QWidget parent;bool seen=false;QTimer timer;connect(&timer,&QTimer::timeout,&parent,[&]{auto d=parent.findChild<QDialog*>();if(!d)return;seen=true;auto combos=d->findChildren<QComboBox*>();QCOMPARE(combos.size(),2);for(auto c:combos)QCOMPARE(c->currentData().toString(),QString("none"));auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())QVERIFY(d->grab().save(dir+"/chimes.png"));auto b=d->findChild<QDialogButtonBox*>();b->button(QDialogButtonBox::Save)->click();});timer.start(200);showChimes(&parent,s);QVERIFY(seen);QCOMPARE(s.get("chime_mic"),QString("none"));QCOMPARE(s.get("chime_ad"),QString("none"));QCOMPARE(s.get("chime_gap"),QString("300"));}

 void timezoneConverted(){auto rows=PrayerService::parseCalendar(calendar(),"Dubai","",QDateTime::fromString("2030-01-01T00:00:00Z",Qt::ISODate));QCOMPARE(rows.size(),5);QCOMPARE(rows[0].due.toString(Qt::ISODate),QString("2030-01-02T08:00:00Z"));QCOMPARE(rows[0].kind,QString("prayer"));}
 void missingTimezoneRejected(){QVERIFY_EXCEPTION_THROWN(PrayerService::parseCalendar(calendar(""),"city","",QDateTime::currentDateTimeUtc()),std::runtime_error);}
 void invalidJsonRejected(){QVERIFY_EXCEPTION_THROWN(PrayerService::parseCalendar("bad","city","",QDateTime::currentDateTimeUtc()),std::runtime_error);}
 void duplicatesRejected(){auto root=QJsonDocument::fromJson(calendar()).object();auto days=root["data"].toArray();days.append(days.first());root["data"]=days;QVERIFY_EXCEPTION_THROWN(PrayerService::parseCalendar(QJsonDocument(root).toJson(),"city","",QDateTime::currentDateTimeUtc()),std::runtime_error);}
 void pastNotScheduled(){QVERIFY(PrayerService::parseCalendar(calendar(),"city","",QDateTime::fromString("2031-01-01T00:00:00Z",Qt::ISODate)).isEmpty());}
 void adhanOptional(){auto rows=PrayerService::parseCalendar(calendar(),"city","adhan.wav",QDateTime::currentDateTimeUtc());QCOMPARE(rows[0].kind,QString("adhan"));QCOMPARE(rows[0].payload,QString("adhan.wav"));}
 void replacementPreservesManualAndHistory(){Store s(":memory:");auto rows=PrayerService::parseCalendar(calendar(),"city","",QDateTime::currentDateTimeUtc());s.add({"manual","tts","hello",rows[0].due,40});PrayerService::saveCalendar(s,rows);s.finish(rows[0].id,"dispatched");PrayerService::saveCalendar(s,rows);auto q=s.query("SELECT count(*) FROM events");QVERIFY(q.next());QCOMPARE(q.value(0).toInt(),6);PrayerService::saveCalendar(s,{});q=s.query("SELECT count(*) FROM events");q.next();QCOMPARE(q.value(0).toInt(),2);}
 void rollbackPreservesSchedule(){Store s(":memory:");auto rows=PrayerService::parseCalendar(calendar(),"city","",QDateTime::currentDateTimeUtc());PrayerService::saveCalendar(s,rows);auto invalid=rows;invalid[1].due=QDateTime();QVERIFY_EXCEPTION_THROWN(PrayerService::saveCalendar(s,invalid),std::runtime_error);auto q=s.query("SELECT count(*) FROM events");q.next();QCOMPARE(q.value(0).toInt(),5);}
 void automaticActivationOnlyOnce(){
  Store s(":memory:");Window w(s);w.show();int shown=0;QTimer watcher;
  connect(&watcher,&QTimer::timeout,&w,[&]{auto d=w.findChild<QDialog*>("activationDialog");if(d&&d->isVisible()){++shown;auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())d->grab().save(dir+"/activation.png");d->reject();}});watcher.start(30);QTest::qWait(2000);QCOMPARE(shown,1);QVERIFY(!w.licenseAllowed());
 }
 void dialogsOpen(){Store s(":memory:");Window w(s);w.show();QTimer dismiss;connect(&dismiss,&QTimer::timeout,&w,[&]{if(auto d=w.findChild<QDialog*>("activationDialog"))d->reject();});dismiss.start(30);QTest::qWait(1000);
 for(const auto &name:QStringList{"prayerSettings","announcementStudio"}){
  bool seen=false;QTimer watcher;connect(&watcher,&QTimer::timeout,&w,[&]{if(auto d=w.findChild<QDialog*>(name)){seen=true;if(name=="prayerSettings"){QVERIFY(d->findChild<QLabel*>("prayerAutomaticSchedule"));const auto boxes=d->findChildren<QCheckBox*>();QCOMPARE(boxes.size(),1);QVERIFY(boxes.first()->isChecked());QVERIFY(!d->findChild<QLabel*>("prayerReadiness")->text().isEmpty());}auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())d->grab().save(dir+"/"+name+".png");d->reject();}});watcher.start(100);
  for(auto b:w.findChildren<QPushButton*>()){bool match=name=="prayerSettings"?b->text()==NText("Prayer times"):b->text().contains(NasaqLanguage()=="ar"?QStringLiteral("استوديو الإعلانات"):QStringLiteral("Announcement studio"));if(match){b->click();break;}}
  QVERIFY2(seen,qPrintable(name));
 }
 }
 void realNetworkSync(){if(!qEnvironmentVariableIsSet("NASAQ_LIVE_NETWORK"))QSKIP("Live network test not requested");Store s(":memory:");PrayerService service(s);QSignalSpy spy(&service,&PrayerService::completed);service.sync("Dubai","AE",16,"");QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(),25000);QVERIFY2(spy.first().first().toBool(),qPrintable(spy.first().at(1).toString()));QCOMPARE(s.get("prayer_timezone"),QString("Asia/Dubai"));auto q=s.query("SELECT count(*) FROM events WHERE status='pending'");q.next();QVERIFY(q.value(0).toInt()>0);}
 void realServiceFixture(){auto path=qEnvironmentVariable("NASAQ_PRAYER_FIXTURE");if(path.isEmpty())QSKIP("No live fixture supplied");QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));auto rows=PrayerService::parseCalendar(file.readAll(),"live","",QDateTime::currentDateTimeUtc());QVERIFY(rows.size()>0);}
};
int main(int argc,char **argv){qputenv("QT_MEDIA_BACKEND","windows");QApplication app(argc,argv);app.setApplicationName("NASAQ-Review-Tests");ReviewTest test;return QTest::qExec(&test,argc,argv);}
#include "review_test.moc"

