// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include "audio.h"
#include "rundown.h"
#include "scheduler.h"
#include "prayers.h"
class BusinessMusic:public MusicTransport{public:bool running=true,held=false;int resumes=0;bool replace(const QStringList&)override{return true;}bool playing()const override{return running;}void suspend(bool on)override{held=on;if(on)running=false;}void resume()override{if(!held){running=true;++resumes;}}void stop()override{running=false;}};
class BusinessTest:public QObject{Q_OBJECT
private slots:
 void earlyPrayerHoldsThroughOriginalEnd(){Store s(":memory:");BusinessMusic m;Audio a(s,nullptr,&m);Scheduler scheduler(s);auto now=QDateTime::currentDateTimeUtc();auto at=now.addSecs(120);s.set("prayer_lead_minutes","2");s.set("prayer_pause_minutes","10");s.add({"fajr","prayer","",at,60});QCOMPARE(PrayerService::nextPause(s,now),now);scheduler.tick(now,[&](const Event&e){return a.dispatch(e);});QVERIFY(m.held);QVERIFY(!a.dispatch({"ad","tts","text",now,40}));QVERIFY(a.playMusic({"replacement.wav"}));QVERIFY(m.held);a.tick(at.addSecs(599));QVERIFY(m.held);a.tick(at.addSecs(600));QVERIFY(!m.held);QCOMPARE(m.resumes,1);}
 void liveRadioHasNoEnd(){auto r=CalculateRundown({60000,180000,60000},0,0,QDateTime::currentDateTime(),true,{1});QVERIFY(r.infinite);QVERIFY(r.remainingInfinite);QCOMPARE(r.unknown,0);QCOMPARE(r.total,120000);QVERIFY(r.ends[0]!=QStringLiteral("—"));QCOMPARE(r.ends[1],QStringLiteral("—"));QCOMPARE(r.ends[2],QStringLiteral("—"));}
 void leavingRadioRestoresFiniteRemaining(){auto r=CalculateRundown({-1,60000},1,10000,QDateTime::currentDateTime(),true,{0});QVERIFY(r.infinite);QVERIFY(!r.remainingInfinite);QCOMPARE(r.remaining,50000);QVERIFY(r.ends[1]!=QStringLiteral("—"));}
 void unknownFileIsNotLiveRadio(){auto r=CalculateRundown({-1},0,0,QDateTime::currentDateTime());QVERIFY(!r.infinite);QCOMPARE(r.unknown,1);}
 void duckReturnsInOneSecond(){double gain=1;MusicDuck d([&](double v){gain=v;});d.press();QCOMPARE(gain,.05);d.release();QTest::qWait(300);QVERIFY(gain>.05&&gain<.6);double mid=gain;QTest::qWait(300);QVERIFY(gain>mid&&gain<1);QTRY_COMPARE_WITH_TIMEOUT(gain,1.,600);}
 void repeatedPressCancelsFade(){double gain=1;MusicDuck d([&](double v){gain=v;});d.press();d.release();QTest::qWait(250);d.press();QCOMPARE(gain,.05);QTest::qWait(1100);QCOMPARE(gain,.05);d.reset();QCOMPARE(gain,1.);}
 void stopCancelsFade(){double gain=1;MusicDuck d([&](double v){gain=v;});d.press();d.release();d.reset();QTest::qWait(1100);QCOMPARE(gain,1.);}
 void totalAndEndTimes(){auto now=QDateTime::fromString("2026-10-17T08:00:00Z",Qt::ISODate);auto r=CalculateRundown({180000,240000,60000},0,60000,now);QCOMPARE(r.total,480000);QCOMPARE(r.remaining,420000);QCOMPARE(r.ends,QStringList({"08:02:00","08:06:00","08:07:00"}));}
 void precedingTracksNotReplayed(){auto now=QDateTime::fromString("2026-10-17T08:00:00Z",Qt::ISODate);auto r=CalculateRundown({180000,240000,60000},1,120000,now);QCOMPARE(r.remaining,180000);QCOMPARE(r.ends[0],QStringLiteral("—"));QCOMPARE(r.ends[2],QString("08:03:00"));}
 void unknownSuppressesFollowingTimes(){auto r=CalculateRundown({60000,-1,60000},0,0,QDateTime::currentDateTime());QCOMPARE(r.unknown,1);QCOMPARE(r.total,120000);QCOMPARE(r.ends[2],QStringLiteral("—"));}
 void unorderedSuppressesTimes(){auto r=CalculateRundown({60000},0,0,QDateTime::currentDateTime(),false);QCOMPARE(r.ends[0],QStringLiteral("—"));}
 void midnightAndLongDuration(){auto now=QDateTime::fromString("2026-10-17T23:59:00Z",Qt::ISODate);auto r=CalculateRundown({120000},0,0,now);QCOMPARE(r.ends[0],QString("2026-10-18 00:01:00"));QCOMPARE(RundownDuration(27LL*3600000),QString("27:00:00"));}
 void reorderChangesEndTimes(){auto now=QDateTime::fromString("2026-10-17T08:00:00Z",Qt::ISODate);auto a=CalculateRundown({60000,180000},0,0,now),b=CalculateRundown({180000,60000},0,0,now);QCOMPARE(a.total,b.total);QCOMPARE(b.ends[0],QString("08:03:00"));QCOMPARE(a.ends[1],b.ends[1]);}
 void emptyAndOvershoot(){QVERIFY(CalculateRundown({},0,0,QDateTime::currentDateTime()).ends.isEmpty());QCOMPARE(CalculateRundown({60000},0,90000,QDateTime::currentDateTime()).remaining,0);}
 void prayerPausesAndReturns(){Store s(":memory:");BusinessMusic m;Audio a(s,nullptr,&m);auto now=QDateTime::currentDateTimeUtc();QVERIFY(a.dispatch({"fajr","prayer",{},now,60}));QVERIFY(m.held&&!m.running);QCOMPARE(a.activePriority(),60);QVERIFY(!a.dispatch({"ad","tts","test",now,40}));a.tick(now.addSecs(599));QVERIFY(m.held);a.tick(now.addSecs(601));QVERIFY(m.running&&!m.held);QCOMPARE(m.resumes,1);}
 void stopDuringPrayerStaysStopped(){Store s(":memory:");BusinessMusic m;Audio a(s,nullptr,&m);auto now=QDateTime::currentDateTimeUtc();a.dispatch({"fajr","prayer",{},now,60});a.stop();a.tick(now.addSecs(601));QVERIFY(!m.running);QCOMPARE(m.resumes,0);}
 void missingAdhanStillPauses(){Store s(":memory:");BusinessMusic m;Audio a(s,nullptr,&m);QVERIFY(!a.dispatch({"fajr","adhan","Z:/missing.wav",QDateTime::currentDateTimeUtc(),60}));QVERIFY(m.held);QCOMPARE(a.activePriority(),60);}
 void disabledPausePreservesMusic(){Store s(":memory:");s.set("prayer_pause_enabled","0");BusinessMusic m;Audio a(s,nullptr,&m);a.dispatch({"fajr","prayer",{},QDateTime::currentDateTimeUtc(),60});QVERIFY(m.running);QVERIFY(!a.prayerActive());}
 void restoresPrayerHoldAfterRestart(){Store s(":memory:");auto now=QDateTime::currentDateTimeUtc();s.set("prayer_hold_until",now.addSecs(60).toString(Qt::ISODate));BusinessMusic m;Audio a(s,nullptr,&m);a.tick(now);QVERIFY(m.held);QCOMPARE(a.activePriority(),60);}
 void schedulerDispatchesPrayerOnce(){Store s(":memory:");BusinessMusic m;Audio a(s,nullptr,&m);Scheduler scheduler(s);auto now=QDateTime::currentDateTimeUtc();s.add({"fajr","prayer",{},now,60});int calls=0;auto dispatch=[&](const Event&e){++calls;return a.dispatch(e);};scheduler.tick(now.addSecs(-1),dispatch);QVERIFY(m.running);scheduler.tick(now,dispatch);scheduler.tick(now.addSecs(1),dispatch);QCOMPARE(calls,1);QVERIFY(m.held);}
};
QTEST_GUILESS_MAIN(BusinessTest)
#include "business_test.moc"
