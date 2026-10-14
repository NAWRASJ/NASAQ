// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include "audio.h"
static bool writeSilence(const QString&path){QFile file(path);if(!file.open(QIODevice::WriteOnly))return false;QDataStream out(&file);out.setByteOrder(QDataStream::LittleEndian);out.writeRawData("RIFF",4);out<<quint32(36+32000);out.writeRawData("WAVEfmt ",8);out<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(16000)<<quint32(32000)<<quint16(2)<<quint16(16);out.writeRawData("data",4);out<<quint32(32000);return file.write(QByteArray(32000,'\0'))==32000;}
class FakeMusic:public MusicTransport {
 public:
 bool running=true,held=false;int resumes=0,stops=0,replacements=0;
 bool replace(const QStringList&files)override{++replacements;return !files.isEmpty();}
 bool playing()const override{return running;}
 void suspend(bool on)override{held=on;if(on)running=false;}
 void resume()override{++resumes;running=true;}
 void stop()override{++stops;running=false;}
};
class AudioTest:public QObject {
 Q_OBJECT
private slots:
 void activationBlocksAllAudio(){Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);audio.setLicenseAllowed(false);QVERIFY(!music.running);QVERIFY(!audio.playMusic({"test"}));QVERIFY(!audio.dispatch({"ad","tts","test",QDateTime::currentDateTimeUtc(),40}));QVERIFY(!audio.page(true));QVERIFY(audio.page(false));audio.stop();QCOMPARE(music.replacements,0);audio.setLicenseAllowed(true);QVERIFY(audio.playMusic({"test"}));}

 void foregroundRestoresMusic(){QTemporaryDir dir;auto path=dir.filePath("silence.wav");QVERIFY(writeSilence(path));Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);QVERIFY(audio.dispatch({"ad","announcement",path,QDateTime::currentDateTimeUtc(),50}));QVERIFY(music.held);QVERIFY(!music.running);QCOMPARE(audio.activePriority(),50);QVERIFY(!audio.dispatch({"low","tts","must not interrupt",QDateTime::currentDateTimeUtc(),40}));QTRY_COMPARE_WITH_TIMEOUT(audio.activePriority(),0,10000);QCOMPARE(music.resumes,1);QVERIFY(!music.held);}
 void stopDoesNotResume(){QTemporaryDir dir;auto path=dir.filePath("silence.wav");QVERIFY(writeSilence(path));Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);QVERIFY(audio.dispatch({"ad","adhan",path,QDateTime::currentDateTimeUtc(),60}));audio.stop();QTest::qWait(300);QVERIFY(!music.running);QCOMPARE(music.resumes,0);QCOMPARE(audio.activePriority(),0);}
 void scheduledReplacementStaysSuspended(){QTemporaryDir dir;auto path=dir.filePath("silence.wav");QVERIFY(writeSilence(path));Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);QVERIFY(audio.dispatch({"ad","announcement",path,QDateTime::currentDateTimeUtc(),50}));QVERIFY(audio.dispatch({"next","music",path,QDateTime::currentDateTimeUtc(),10}));QVERIFY(music.held);QCOMPARE(music.replacements,1);QTRY_COMPARE_WITH_TIMEOUT(audio.activePriority(),0,10000);QCOMPARE(music.resumes,1);}
 void injectedTransportStop(){Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);QVERIFY(audio.playMusic({"delegated-to-strawberry"}));QCOMPARE(music.replacements,1);audio.stop();QCOMPARE(music.stops,1);QVERIFY(!music.held);QCOMPARE(music.resumes,0);}
 void invalidOverlayPreservesMusic(){Store s(":memory:");FakeMusic music;Audio audio(s,nullptr,&music);QVERIFY(!audio.dispatch({"a","adhan","Z:/CAM-NONEXISTENT.wav",QDateTime::currentDateTimeUtc(),60}));QVERIFY(music.running);QVERIFY(!music.held);QCOMPARE(audio.activePriority(),0);}
 void invalidFile(){Store s(":memory:");Audio audio(s);QVERIFY(!audio.playMusic({"Z:/CAM-NONEXISTENT.wav"}));QVERIFY(!audio.dispatch({"a","announcement","Z:/CAM-NONEXISTENT.wav",QDateTime::currentDateTimeUtc(),50}));}
 void prayerWithoutAdhan(){Store s(":memory:");Audio audio(s);Event e{"fajr","prayer",{},QDateTime::currentDateTimeUtc(),60};s.add(e);QVERIFY(audio.dispatch(e));auto q=s.query("SELECT kind FROM logs");QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("prayer_reminder"));}
 void silentWavPlayback(){
  QTemporaryDir dir;auto path=dir.filePath("silence.wav");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
  QDataStream out(&file);out.setByteOrder(QDataStream::LittleEndian);out.writeRawData("RIFF",4);out<<quint32(36+32000);out.writeRawData("WAVEfmt ",8);out<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(16000)<<quint32(32000)<<quint16(2)<<quint16(16);out.writeRawData("data",4);out<<quint32(32000);file.write(QByteArray(32000,'\0'));file.close();
  Store s(":memory:");Audio audio(s);QSignalSpy spy(&audio,&Audio::status);QVERIFY(audio.playMusic({path}));
  auto started=[&]{auto q=s.query("SELECT count(*) FROM logs WHERE kind='play_started'");q.next();return q.value(0).toInt()>0;};QTRY_VERIFY_WITH_TIMEOUT(started(),10000);audio.stop();
 }
};
int main(int argc,char**argv){qputenv("QT_MEDIA_BACKEND","windows");QCoreApplication app(argc,argv);AudioTest test;return QTest::qExec(&test,argc,argv);}
#include "audio_test.moc"
