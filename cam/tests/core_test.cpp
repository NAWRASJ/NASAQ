// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include "scheduler.h"
class CoreTest:public QObject {
 Q_OBJECT
private slots:
 void prayerAfterSleepRetainsRemainingWindow(){Store s(":memory:");auto now=QDateTime::currentDateTimeUtc();s.set("prayer_pause_minutes","10");s.add({"prayer","adhan","sound.mp3",now.addSecs(-120),60});s.add({"expired","prayer","",now.addSecs(-600),60});s.add({"old-ad","announcement","sound.mp3",now.addSecs(-120),50});auto rows=s.due(now);QCOMPARE(rows.size(),1);QCOMPARE(rows.first().id,QString("prayer"));QCOMPARE(rows.first().kind,QString("prayer"));QCOMPARE(rows.first().due,now.addSecs(-120));}
 void once(){Store s(":memory:");Scheduler scheduler(s);auto now=QDateTime::currentDateTimeUtc();s.add({"a","tts","hello",now,40});int count=0;auto dispatch=[&](const Event&){++count;return true;};scheduler.tick(now,dispatch);scheduler.tick(now,dispatch);QCOMPARE(count,1);}
 void ordering(){Store s(":memory:");Scheduler t(s);auto n=QDateTime::currentDateTimeUtc();s.add({"music","music","a",n,10});s.add({"prayer","adhan","b",n,60});QStringList ids;t.tick(n,[&](const Event&e){ids<<e.id;return true;});QCOMPARE(ids,QStringList({"prayer","music"}));}
 void staleAndFuture(){Store s(":memory:");auto n=QDateTime::currentDateTimeUtc();s.add({"old","tts","a",n.addSecs(-61),40});s.add({"future","tts","a",n.addSecs(1),40});QVERIFY(s.due(n).isEmpty());auto q=s.query("SELECT status FROM events WHERE id='old'");QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("missed"));}
 void crashRecovery(){QTemporaryDir dir;auto path=dir.filePath("db.sqlite");auto n=QDateTime::currentDateTimeUtc();{Store s(path);s.add({"a","tts","a",n,40});QVERIFY(s.claim("a"));}{Store s(path);QVERIFY(s.due(n).isEmpty());auto q=s.query("SELECT status FROM events");QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("interrupted"));}}
 void rejection(){Store s(":memory:");Scheduler t(s);auto n=QDateTime::currentDateTimeUtc();s.add({"a","tts","a",n,40});t.tick(n,[](const Event&){return false;});auto q=s.query("SELECT status FROM events");q.next();QCOMPARE(q.value(0).toString(),QString("rejected"));}
 void csv(){QCOMPARE(Store::csvCell("=SUM(A1)"),QString("\"'=SUM(A1)\""));QCOMPARE(Store::csvCell("a\"b"),QString("\"a\"\"b\""));Store s(":memory:");s.log("test","مرحبا, world");QTemporaryDir dir;s.exportCsv(dir.filePath("out.csv"));QFile f(dir.filePath("out.csv"));QVERIFY(f.open(QIODevice::ReadOnly));QVERIFY(f.readAll().contains(QString("مرحبا").toUtf8()));}
 void duplicate(){Store s(":memory:");Event e{"a","tts","a",QDateTime::currentDateTimeUtc(),40};s.add(e);QVERIFY_EXCEPTION_THROWN(s.add(e),std::runtime_error);}
};
QTEST_GUILESS_MAIN(CoreTest)
#include "core_test.moc"
