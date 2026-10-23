// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QtWidgets>
#include "cloudspeech.h"
#include "cloudspeechdialog.h"
#include "nasaqtheme.h"
#include <cstring>
static QByteArray wav(){QByteArray bytes=QByteArray::fromHex("524946462600000057415645666d74201000000001000100c05d000080bb00000200100064617461020000000100");return bytes;}
class Reply:public QNetworkReply{
 QByteArray data; qint64 position=0;
public:
 Reply(const QNetworkRequest &request,QByteArray body,int status,bool delayed,QObject *parent):QNetworkReply(parent),data(body){setRequest(request);setUrl(request.url());setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);open(QIODevice::ReadOnly);if(!delayed)QTimer::singleShot(0,this,[this]{if(isFinished())return;emit readyRead();setFinished(true);emit finished();});}
 void abort() override {if(isFinished())return;setError(OperationCanceledError,"cancelled");setFinished(true);emit finished();}
 qint64 bytesAvailable()const override{return data.size()-position+QNetworkReply::bytesAvailable();}
 qint64 readData(char *out,qint64 max)override{auto n=qMin(max,qint64(data.size())-position);if(!n)return -1;memcpy(out,data.constData()+position,size_t(n));position+=n;return n;}
};
class Network:public QNetworkAccessManager{
public:
 int calls=0,status=200;bool delayed=false;QByteArray response=QJsonDocument(QJsonObject{{"audioContent",QString::fromLatin1(wav().toBase64())}}).toJson(),posted;QNetworkRequest last;
protected:
 QNetworkReply *createRequest(Operation,const QNetworkRequest &request,QIODevice *body)override{++calls;last=request;posted=body?body->readAll():QByteArray();return new Reply(request,response,status,delayed,this);}
};
class CloudTest:public QObject{
 Q_OBJECT
private slots:
 void init(){QSettings().clear();}
 void cleanupTestCase(){QSettings().clear();}
 void payloadAndArabicByteLimit(){auto body=CloudSpeech::requestBody(QStringLiteral("مرحبا"),"ar-XA","ar-XA-Chirp3-HD-Aoede");QCOMPARE(body["voice"].toObject()["languageCode"].toString(),QString("ar-XA"));QCOMPARE(body["audioConfig"].toObject()["audioEncoding"].toString(),QString("LINEAR16"));QVERIFY_EXCEPTION_THROWN(CloudSpeech::requestBody(QString(2501,QChar(0x0627)),"ar-XA","ar-XA-Chirp3-HD-Aoede"),std::runtime_error);QVERIFY_EXCEPTION_THROWN(CloudSpeech::requestBody(" ","ar-XA","ar-XA-Chirp3-HD-Aoede"),std::runtime_error);QVERIFY_EXCEPTION_THROWN(CloudSpeech::requestBody("Hi","en-US","ar-XA-Chirp3-HD-Aoede"),std::runtime_error);}
 void rejectsMalformedAudio(){QVERIFY(CloudSpeech::validWave(wav()));QVERIFY_EXCEPTION_THROWN(CloudSpeech::decodeAudio("{}"),std::runtime_error);QVERIFY_EXCEPTION_THROWN(CloudSpeech::decodeAudio("{\"audioContent\":\"%%%\"}"),std::runtime_error);auto broken=wav();broken[4]=0;QVERIFY(!CloudSpeech::validWave(broken));QVERIFY(!CloudSpeech::validWave(wav().left(40)));}
 void generatesThenReusesOffline(){QTemporaryDir dir;Network network;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy ready(&service,&CloudSpeech::ready);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","test-token");QTRY_COMPARE(ready.size(),1);QCOMPARE(network.calls,1);QCOMPARE(network.last.url().toString(),QString("https://texttospeech.googleapis.com/v1/text:synthesize"));QCOMPARE(network.last.rawHeader("Authorization"),QByteArray("Bearer test-token"));QVERIFY(!network.posted.contains("test-token"));QCOMPARE(network.last.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),int(QNetworkRequest::ManualRedirectPolicy));auto file=ready.first()[0].toString();QFile saved(file);QVERIFY(saved.open(QIODevice::ReadOnly));QCOMPARE(saved.readAll(),wav());saved.close();service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","","");QCOMPARE(ready.size(),2);QVERIFY(ready.last()[1].toBool());QCOMPARE(network.calls,1);for(const auto &key:QSettings().allKeys())QVERIFY(!QSettings().value(key).toString().contains("test-token"));}
 void noTokenNoRequest(){QTemporaryDir dir;Network network;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy failed(&service,&CloudSpeech::failed);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","");QCOMPARE(failed.first()[0].toString(),QString("TokenRequired"));QCOMPARE(network.calls,0);}
 void errorsDoNotSave_data(){QTest::addColumn<int>("status");QTest::addColumn<QString>("code");QTest::newRow("expired")<<401<<QString("Authentication");QTest::newRow("billing-or-permission")<<403<<QString("Permission");QTest::newRow("quota")<<429<<QString("Quota");QTest::newRow("server")<<503<<QString("Network");QTest::newRow("redirect")<<302<<QString("Network");}
 void errorsDoNotSave(){QFETCH(int,status);QFETCH(QString,code);QTemporaryDir dir;Network network;network.status=status;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy failed(&service,&CloudSpeech::failed);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","test-token");QTRY_COMPARE(failed.size(),1);QCOMPARE(failed.first()[0].toString(),code);QCOMPARE(network.calls,1);QVERIFY(QDir(dir.path()).entryList(QDir::Files).isEmpty());}
 void cancelledRequestDoesNotSave(){QTemporaryDir dir;Network network;network.delayed=true;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy failed(&service,&CloudSpeech::failed);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","test-token");QVERIFY(service.busy());service.cancel();QCOMPARE(failed.size(),1);QCOMPARE(failed.first()[0].toString(),QString("Cancelled"));QVERIFY(!service.busy());QVERIFY(QDir(dir.path()).entryList(QDir::Files).isEmpty());}
 void capPreventsRequest(){QSettings().setValue("cloudSpeech/usage/nasaq-test/"+QDate::currentDate().toString("yyyy-MM"),99999);QTemporaryDir dir;Network network;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy failed(&service,&CloudSpeech::failed);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","test-token");QCOMPARE(failed.first()[0].toString(),QString("LocalLimit"));QCOMPARE(network.calls,0);}
 void corruptCacheIsNotPlayed(){QTemporaryDir dir;Network network;CloudSpeech service(dir.path(),nullptr,&network);QSignalSpy ready(&service,&CloudSpeech::ready);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","test-token");QTRY_COMPARE(ready.size(),1);QFile file(ready.first()[0].toString());QVERIFY(file.open(QIODevice::WriteOnly));file.write("broken");file.close();QSignalSpy failed(&service,&CloudSpeech::failed);service.generate("Hello","en-US","en-US-Chirp3-HD-Aoede","nasaq-test","");QCOMPARE(failed.size(),1);QCOMPARE(ready.size(),1);}
 void dialogNoCredentials(){bool seen=false;QTimer timer;connect(&timer,&QTimer::timeout,this,[&]{for(auto widget:QApplication::topLevelWidgets()){if(widget->objectName()!="cloudSpeechDialog")continue;auto dialog=qobject_cast<QDialog*>(widget);if(!dialog)return;seen=true;dialog->findChild<QLineEdit*>("cloudProject")->setText("nasaq-test");dialog->findChild<QPlainTextEdit*>("cloudText")->setPlainText(QStringLiteral("مرحبًا بكم"));dialog->findChild<QPushButton*>("cloudGenerate")->click();QVERIFY(!dialog->findChild<QPushButton*>("cloudUse")->isEnabled());QVERIFY(!dialog->findChild<QLabel*>("cloudStatus")->text().isEmpty());auto path=qEnvironmentVariable("NASAQ_CLOUD_EVIDENCE");if(!path.isEmpty())dialog->grab().save(path+"/cloud-dialog.png");dialog->reject();}});timer.start(100);QVERIFY(createCloudAnnouncement(nullptr).isEmpty());QVERIFY(seen);}
};
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("NASAQ-Test");app.setApplicationName("Cloud-"+QUuid::createUuid().toString(QUuid::WithoutBraces));ApplyNasaqTheme();CloudTest test;return QTest::qExec(&test,argc,argv);}
#include "cloud_test.moc"
