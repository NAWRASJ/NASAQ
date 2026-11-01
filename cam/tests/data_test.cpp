// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QJsonArray>
#include "datamanager.h"
#include "store.h"
#include "licenselease.h"
class DataTest:public QObject {
 Q_OBJECT
 QTemporaryDir temp;QString root,archive;QByteArray sealed;
 static QByteArray read(QString path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}
 static void write(QString path,QByteArray b){QFile f(path);if(!f.open(QIODevice::WriteOnly)||f.write(b)!=b.size())throw std::runtime_error("fixture write failed");}
 QJsonObject document(){return QJsonDocument::fromJson(unprotectLicenseData(read(archive).mid(15))).object();}
 void document(QJsonObject o){write(archive,"NASAQ-BACKUP-1\n"+protectLicenseData(QJsonDocument(o).toJson()));}
 void seed(){
  QVERIFY(QDir().mkpath(root+"/CAM"));
  {Store s(root+"/CAM/cam.sqlite");s.set("activation_sealed_v1",QString::fromLatin1(sealed.toBase64()));s.set("city","Abu Dhabi");s.add({"future","announcement","hello",QDateTime::currentDateTimeUtc().addDays(2),10});}
  {Store library(root+"/strawberry.db");library.exec("CREATE TABLE playlists(name TEXT)");library.exec("INSERT INTO playlists VALUES('Keep me')");}
  write(root+"/asset.txt","imported asset");
  QSettings().setValue("volume",63);QSettings(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences").setValue("language","ar");
 }
 private slots:
 void initTestCase(){QVERIFY(temp.isValid());QCoreApplication::setOrganizationName("NASAQ-Data-Test");QCoreApplication::setApplicationName("Isolated");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path()+"/settings");sealed=protectLicenseData("test-license-secret");QVERIFY(!sealed.isEmpty());}
 void init(){root=temp.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));archive=root+".nasaqbackup";QSettings().clear();QSettings(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences").clear();seed();}
 void roundTripIncludesSettingsLicenseAssets(){
  NasaqData::backup(root,archive);QVERIFY(!read(archive).contains("test-license-secret"));QVERIFY(!read(archive).contains("imported asset"));
  {Store s(root+"/CAM/cam.sqlite");s.set("city","changed");s.set("activation_sealed_v1","");}
  write(root+"/asset.txt","changed");QSettings().setValue("volume",9);QSettings(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences").setValue("language","en");
  NasaqData::queueRestore(root,archive);QVERIFY(NasaqData::applyPending(root));QVERIFY(!NasaqData::applyPending(root));
  QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));QCOMPARE(QSettings().value("volume").toInt(),63);QCOMPARE(QSettings(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences").value("language").toString(),QString("ar"));
  Store s(root+"/CAM/cam.sqlite");QCOMPARE(s.get("city"),QString("Abu Dhabi"));QCOMPARE(unprotectLicenseData(QByteArray::fromBase64(s.get("activation_sealed_v1").toLatin1())),QByteArray("test-license-secret"));auto q=s.query("SELECT count(*) FROM events");QVERIFY(q.next());QCOMPARE(q.value(0).toInt(),1);
 }
 void backupIncludesLiveWalTransactions(){
  {Store live(root+"/CAM/cam.sqlite");live.set("live-wal","latest committed value");NasaqData::backup(root,archive);live.set("live-wal","changed after backup");}
  NasaqData::queueRestore(root,archive);QVERIFY(NasaqData::applyPending(root));Store restored(root+"/CAM/cam.sqlite");QCOMPARE(restored.get("live-wal"),QString("latest committed value"));
 }
 void committedCleanupNeverRollsBack(){
  QJsonObject j{{"profile",root},{"settings",QJsonObject()},{"preferences",QJsonObject()},{"committed",true}};
  write(root+".nasaq-transaction",protectLicenseData(QJsonDocument(j).toJson()));QVERIFY(QDir().mkpath(root+".nasaq-previous"));write(root+".nasaq-previous/partial","old partially removed data");
  QVERIFY(NasaqData::applyPending(root));QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));QVERIFY(!QFileInfo::exists(root+".nasaq-previous"));QCOMPARE(QSettings().value("volume").toInt(),63);
 }
 void restoreKeepsNewerCurrentLicense(){NasaqData::backup(root,archive);{Store s(root+"/CAM/cam.sqlite");s.set("activation_sealed_v1","newer");}NasaqData::queueRestore(root,archive);QVERIFY(NasaqData::applyPending(root));Store s(root+"/CAM/cam.sqlite");QCOMPARE(s.get("activation_sealed_v1"),QString("newer"));}
 void restoreSelectedLicense(){NasaqData::backup(root,archive);{Store s(root+"/CAM/cam.sqlite");s.set("activation_sealed_v1","newer");}NasaqData::queueRestore(root,archive,false);QVERIFY(NasaqData::applyPending(root));Store s(root+"/CAM/cam.sqlite");QCOMPARE(s.get("activation_sealed_v1"),QString::fromLatin1(sealed.toBase64()));}
 void resetPreservesLicenseAndExternalFiles(){write(root+".external.mp3","external");NasaqData::backup(root,archive);NasaqData::queueReset(root);QVERIFY(NasaqData::applyPending(root));QVERIFY(!QFileInfo::exists(root+"/asset.txt"));QVERIFY(QSettings().allKeys().isEmpty());QVERIFY(QSettings(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences").allKeys().isEmpty());QCOMPARE(read(root+".external.mp3"),QByteArray("external"));QVERIFY(QFileInfo::exists(archive));Store s(root+"/CAM/cam.sqlite");QVERIFY(s.get("city").isEmpty());QCOMPARE(s.get("activation_sealed_v1"),QString::fromLatin1(sealed.toBase64()));}
 void resetDeletesLicenseWhenExplicitlySelected(){NasaqData::queueReset(root,false);QVERIFY(NasaqData::applyPending(root));Store s(root+"/CAM/cam.sqlite");QVERIFY(s.get("activation_sealed_v1").isEmpty());}
 void tamperingRejected(){NasaqData::backup(root,archive);auto b=read(archive);b[b.size()/2]^=1;write(archive,b);QVERIFY_EXCEPTION_THROWN(NasaqData::queueRestore(root,archive),std::exception);QVERIFY(!QFileInfo::exists(root+"/.nasaq-maintenance.json"));QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));}
 void changedBackupCancelsSafely(){NasaqData::backup(root,archive);NasaqData::queueRestore(root,archive);write(archive,"broken");QVERIFY_EXCEPTION_THROWN(NasaqData::applyPending(root),std::exception);QVERIFY(!NasaqData::applyPending(root));QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));}
 void traversalRejected(){NasaqData::backup(root,archive);auto o=document();auto files=o["files"].toArray();auto f=files[0].toObject();f["name"]="../outside";files[0]=f;o["files"]=files;document(o);QVERIFY_EXCEPTION_THROWN(NasaqData::validateBackup(archive),std::exception);}
 void corruptedDatabaseDoesNotReplaceProfile(){NasaqData::backup(root,archive);auto o=document();auto files=o["files"].toArray();for(int i=0;i<files.size();++i){auto f=files[i].toObject();if(f["name"].toString()=="strawberry.db"){QByteArray b="not sqlite";f["data"]=QString::fromLatin1(b.toBase64());f["sha256"]=QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());files[i]=f;}}o["files"]=files;document(o);NasaqData::queueRestore(root,archive);QVERIFY_EXCEPTION_THROWN(NasaqData::applyPending(root),std::exception);QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));QCOMPARE(QSettings().value("volume").toInt(),63);QVERIFY(!QFileInfo::exists(root+".nasaq-transaction"));}
 void backupInsideProfileRejected(){QVERIFY_EXCEPTION_THROWN(NasaqData::backup(root,root+"/bad.nasaqbackup"),std::exception);}
 void otherProfileRejected(){NasaqData::backup(root,archive);auto o=document();o["originalRoot"]="C:/different-profile";document(o);QVERIFY_EXCEPTION_THROWN(NasaqData::queueRestore(root,archive),std::exception);}
 void interruptedDirectorySwapRecovers(){
  QJsonObject j{{"profile",root},{"settings",QJsonObject()},{"preferences",QJsonObject()},{"committed",false}};
  write(root+".nasaq-transaction",protectLicenseData(QJsonDocument(j).toJson()));QVERIFY(QDir().rename(root,root+".nasaq-previous"));QVERIFY(QDir().mkpath(root));write(root+"/partial","incomplete");
  QVERIFY(NasaqData::applyPending(root));QCOMPARE(read(root+"/asset.txt"),QByteArray("imported asset"));QVERIFY(!QFileInfo::exists(root+"/partial"));QVERIFY(!QFileInfo::exists(root+".nasaq-stage"));
 }
};
QTEST_MAIN(DataTest)
#include "data_test.moc"
