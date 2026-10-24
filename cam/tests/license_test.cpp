// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QNetworkReply>
#include <QJsonArray>
#include <openssl/evp.h>
#include "licenseactivation.h"
#include "activationdialog.h"
class MockReply:public QNetworkReply {
 QByteArray bytes; qint64 pos=0;
 public:
 MockReply(const QNetworkRequest&r,int status,const QJsonObject&body,QObject*p):QNetworkReply(p),bytes(QJsonDocument(body).toJson()){
  setRequest(r);setUrl(r.url());setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);open(QIODevice::ReadOnly);
  QTimer::singleShot(0,this,[this]{setFinished(true);Q_EMIT readyRead();Q_EMIT finished();});
 }
 void abort()override{}qint64 bytesAvailable()const override{return bytes.size()-pos+QNetworkReply::bytesAvailable();}
 protected:qint64 readData(char*target,qint64 max)override{auto n=qMin(max,bytes.size()-pos);if(n<=0)return -1;memcpy(target,bytes.constData()+pos,n);pos+=n;return n;}
};
class MockNetwork:public QNetworkAccessManager {
 public:QList<QPair<int,QJsonObject>> replies;QList<QUrl> urls;QList<QJsonObject> bodies;
 protected:QNetworkReply*createRequest(Operation,const QNetworkRequest&r,QIODevice*out)override{
  urls<<r.url();bodies<<QJsonDocument::fromJson(out?out->readAll():QByteArray()).object();
  auto result=replies.isEmpty()?qMakePair(0,QJsonObject()):replies.takeFirst();return new MockReply(r,result.first,result.second,this);
 }
};
class LicenseTest:public QObject {
 Q_OBJECT
 EVP_PKEY*signer=nullptr;LicenseConfig c;QJsonObject data;QDateTime now;
 QJsonObject relation(QString id){return {{"data",QJsonObject{{"id",id}}}};}
 QByteArray sign(const QJsonObject&doc){
  auto enc=QJsonDocument(doc).toJson(QJsonDocument::Compact).toBase64(),msg="machine/"+enc;size_t n=64;QByteArray sig(64,0);auto ctx=EVP_MD_CTX_new();
  EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,signer);EVP_DigestSign(ctx,reinterpret_cast<unsigned char*>(sig.data()),&n,reinterpret_cast<const unsigned char*>(msg.constData()),msg.size());EVP_MD_CTX_free(ctx);sig.resize(n);
  auto envelope=QJsonDocument(QJsonObject{{"enc",QString::fromLatin1(enc)},{"sig",QString::fromLatin1(sig.toBase64())},{"alg","base64+ed25519"}}).toJson(QJsonDocument::Compact);
  return "-----BEGIN MACHINE FILE-----\n"+envelope.toBase64()+"\n-----END MACHINE FILE-----\n";
 }
 LicenseLease verify(const QJsonObject&doc){return verifyLicenseLease(sign(doc),c,licenseFingerprint(),now);}
 QJsonObject validation(bool ok,QString code={}){return {{"meta",QJsonObject{{"valid",ok},{"code",code}}},{"data",data["included"].toArray()[0]}};}
 QJsonObject checkout(){return {{"data",QJsonObject{{"attributes",QJsonObject{{"certificate",QString::fromUtf8(sign(data))}}}}}};}
 void seed(Store&s){auto cache=QJsonDocument(QJsonObject{{"certificate",QString::fromUtf8(sign(data))},{"lastSeen",now.toString(Qt::ISODate)}}).toJson();s.set("activation_sealed_v1",protectLicenseData(cache).toBase64());}
 private slots:
 void initTestCase(){auto ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);QVERIFY(ctx);QCOMPARE(EVP_PKEY_keygen_init(ctx),1);QCOMPARE(EVP_PKEY_keygen(ctx,&signer),1);EVP_PKEY_CTX_free(ctx);size_t n=32;QByteArray pub(32,0);QCOMPARE(EVP_PKEY_get_raw_public_key(signer,reinterpret_cast<unsigned char*>(pub.data()),&n),1);c={"account-test","product-test",QString::fromLatin1(pub.toHex())};QVERIFY(!licenseFingerprint().isEmpty());}
 void cleanupTestCase(){EVP_PKEY_free(signer);}
 void init(){
  now=QDateTime::currentDateTimeUtc();
  QJsonObject rel{{"account",relation(c.account)},{"product",relation(c.product)}};
  QJsonObject license{{"id","license-test"},{"type","licenses"},{"relationships",rel},{"attributes",QJsonObject{{"key","ABCD2345EFGH6789"},{"expiry",now.addDays(30).toString(Qt::ISODate)},{"status","ACTIVE"},{"suspended",false},{"maxMachines",2}}}};
  rel["license"]=relation("license-test");
  data={{"data",QJsonObject{{"id","machine-test"},{"type","machines"},{"relationships",rel},{"attributes",QJsonObject{{"fingerprint",licenseFingerprint()}}}}},{"included",QJsonArray{license}},{"meta",QJsonObject{{"issued",now.addSecs(-10).toString(Qt::ISODate)},{"expiry",now.addDays(7).toString(Qt::ISODate)},{"ttl",604800}}}};
 }
 void validSignedLease(){auto r=verify(data);QVERIFY2(r.valid,qPrintable(r.message));QCOMPARE(r.maxMachines,2);QVERIFY(r.until<=now.addDays(7));QCOMPARE(r.expiresAt.toSecsSinceEpoch(),now.addDays(30).toSecsSinceEpoch());}
 void localDeactivationPersistsAndAllowsDifferentKey(){
  Store s(":memory:");seed(s);s.set("unrelated-setting","preserved");MockNetwork net;Activation a(s,nullptr,c,&net);QVERIFY(a.allowed());QSignalSpy changed(&a,&Activation::changed);
  QVERIFY(a.deactivateLocally());QVERIFY(!a.allowed());QVERIFY(s.get("activation_sealed_v1").isEmpty());QCOMPARE(s.get("unrelated-setting"),QString("preserved"));QCOMPARE(changed.count(),1);QVERIFY(!changed[0][0].toBool());
  a.tick();QVERIFY(net.urls.isEmpty());Activation restarted(s,nullptr,c,&net);QVERIFY(!restarted.allowed());restarted.tick();QVERIFY(net.urls.isEmpty());
  auto included=data["included"].toArray();auto license=included[0].toObject();auto attrs=license["attributes"].toObject();attrs["key"]="2345ABCDEFGH6789";license["attributes"]=attrs;included[0]=license;data["included"]=included;
  net.replies={{200,validation(true)},{200,checkout()}};restarted.activate("2345ABCDEFGH6789");QTRY_VERIFY(restarted.allowed());QCOMPARE(restarted.details().key,QString("2345ABCDEFGH6789"));
 }
 void localDeactivationWaitsForRenewal(){
  Store s(":memory:");seed(s);MockNetwork net;net.replies={{200,validation(true)},{200,checkout()}};Activation a(s,nullptr,c,&net);QSignalSpy done(&a,&Activation::completed);a.tick();QVERIFY(!a.deactivateLocally());QVERIFY(a.allowed());QTRY_COMPARE(done.count(),1);QVERIFY(a.deactivateLocally());a.tick();QVERIFY(!a.allowed());QCOMPARE(net.urls.size(),2);
 }
 void deactivationDialogCancelAndConfirm(){
  Store s(":memory:");seed(s);MockNetwork net;Activation a(s,nullptr,c,&net);ActivationDialog dialog(a);dialog.show();auto button=dialog.findChild<QPushButton*>("deactivateLicense");QVERIFY(button->isVisible());
  QTimer::singleShot(0,[]{auto box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(box)box->button(QMessageBox::No)->click();});button->click();QVERIFY(a.allowed());
  QTimer::singleShot(0,[]{auto box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(box)box->button(QMessageBox::Yes)->click();});button->click();QVERIFY(!a.allowed());QVERIFY(button->isHidden());QVERIFY(dialog.findChild<QLineEdit*>("activationKey")->isVisible());QVERIFY(dialog.findChild<QPushButton*>("activateLicense")->isVisible());
  auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())dialog.grab().save(dir+"/license-deactivated.png");
 }
 void activationDialogConfirmsAndReopens(){
  Store s(":memory:");MockNetwork net;net.replies={{200,validation(true)},{200,checkout()}};Activation a(s,nullptr,c,&net);
  ActivationDialog d(a);d.show();auto key=d.findChild<QLineEdit*>("activationKey");auto button=d.findChild<QPushButton*>("activateLicense");key->setText("ABCD2345EFGH6789");button->click();
  QTRY_VERIFY(a.allowed());QVERIFY(d.isVisible());QVERIFY(key->isHidden());QVERIFY(button->isHidden());
  auto text=d.findChild<QLabel*>("activationMessage")->text();QVERIFY(text.contains(NasaqLanguage()=="ar"?QStringLiteral("تم التفعيل بنجاح"):QStringLiteral("Activation completed successfully")));QVERIFY(text.contains(a.details().expiresAt.toLocalTime().toString("yyyy-MM-dd HH:mm")));QVERIFY(text.contains(a.details().until.toLocalTime().toString("yyyy-MM-dd HH:mm")));QVERIFY(!text.contains("ABCD2345"));
  d.close();Activation restarted(s,nullptr,c,&net);ActivationDialog reopened(restarted);reopened.show();QVERIFY(reopened.findChild<QLineEdit*>("activationKey")->isHidden());QVERIFY(reopened.findChild<QPushButton*>("activateLicense")->isHidden());QCOMPARE(net.urls.size(),2);
  auto dir=qEnvironmentVariable("NASAQ_REVIEW_EVIDENCE");if(!dir.isEmpty())reopened.grab().save(dir+"/license-active.png");
 }
 void activationDialogInvalidKeyStaysOpen(){Store s(":memory:");MockNetwork net;Activation a(s,nullptr,c,&net);ActivationDialog d(a);d.show();d.findChild<QLineEdit*>("activationKey")->setText("1234");auto button=d.findChild<QPushButton*>("activateLicense");button->click();QVERIFY(d.isVisible());QVERIFY(button->isEnabled());QVERIFY(!a.allowed());QVERIFY(net.urls.isEmpty());}
 void activationFailureIsNotSuccessDuringGrace(){Store s(":memory:");seed(s);MockNetwork net;net.replies={{0,{}}};Activation a(s,nullptr,c,&net);QSignalSpy done(&a,&Activation::completed);a.activate("ABCD2345EFGH6789");QTRY_COMPARE(done.count(),1);QVERIFY(!done[0][0].toBool());QVERIFY(a.allowed());}
 void tamperingRejected(){auto cert=sign(data);cert[70]=cert[70]=='A'?'B':'A';QVERIFY(!verifyLicenseLease(cert,c,licenseFingerprint(),now).valid);auto other=c;other.publicKey=QString(64,'0');QVERIFY(!verifyLicenseLease(sign(data),other,licenseFingerprint(),now).valid);}
 void wrongIdentityRejected(){QVERIFY(!verifyLicenseLease(sign(data),c,"other-device",now).valid);auto other=c;other.product="other";QVERIFY(!verifyLicenseLease(sign(data),other,licenseFingerprint(),now).valid);other=c;other.account="other";QVERIFY(!verifyLicenseLease(sign(data),other,licenseFingerprint(),now).valid);}
 void expiryAndGrace(){QVERIFY(!verifyLicenseLease(sign(data),c,licenseFingerprint(),now.addDays(8)).valid);auto arr=data["included"].toArray();auto l=arr[0].toObject(),a=l["attributes"].toObject();a["expiry"]=now.addSecs(-1).toString(Qt::ISODate);l["attributes"]=a;arr[0]=l;data["included"]=arr;QVERIFY(!verify(data).valid);}
 void suspensionAndClock(){QVERIFY(!verifyLicenseLease(sign(data),c,licenseFingerprint(),now,now.addSecs(600)).valid);auto arr=data["included"].toArray();auto l=arr[0].toObject(),a=l["attributes"].toObject();a["suspended"]=true;l["attributes"]=a;arr[0]=l;data["included"]=arr;QVERIFY(!verify(data).valid);}
 void missingConfiguration(){QVERIFY(!verifyLicenseLease(sign(data),{},licenseFingerprint(),now).valid);Store s(":memory:");MockNetwork network;Activation a(s,nullptr,{},&network);a.activate("ABCD2345EFGH6789");QVERIFY(!a.allowed());QVERIFY(network.urls.isEmpty());}
 void keyFormatting(){QCOMPARE(normalizeLicenseKey("abcd-2345-efgh-6789"),QString("ABCD2345EFGH6789"));QVERIFY(normalizeLicenseKey("1234").isEmpty());QVERIFY(normalizeLicenseKey("OOOOOOOOOOOOOOOO").isEmpty());}
 void protectedCache(){auto plain=sign(data),sealed=protectLicenseData(plain);QVERIFY(!sealed.isEmpty());QVERIFY(!sealed.contains("ABCD2345"));QCOMPARE(unprotectLicenseData(sealed),plain);sealed[20]=char(sealed[20]^1);QVERIFY(unprotectLicenseData(sealed).isEmpty());}
 void firstActivationAndRestart(){Store s(":memory:");MockNetwork net;net.replies={{200,validation(false,"NO_MACHINES")},{201,{}},{200,validation(true)},{200,checkout()}};Activation a(s,nullptr,c,&net);a.activate("ABCD-2345-EFGH-6789");QTRY_VERIFY(a.allowed());QCOMPARE(net.urls.size(),4);QVERIFY(net.urls[1].path().endsWith("/machines"));QVERIFY(!s.get("activation_sealed_v1").contains("ABCD2345"));Activation restarted(s,nullptr,c,&net);QVERIFY(restarted.allowed());}
 void deviceLimitDenied(){Store s(":memory:");MockNetwork net;net.replies={{200,validation(false,"NO_MACHINE")},{422,{}},{200,validation(false,"TOO_MANY_MACHINES")}};Activation a(s,nullptr,c,&net);a.activate("ABCD2345EFGH6789");QTRY_COMPARE(net.urls.size(),3);QTest::qWait(20);QVERIFY(!a.allowed());QVERIFY(s.get("activation_sealed_v1").isEmpty()||!unprotectLicenseData(QByteArray::fromBase64(s.get("activation_sealed_v1").toLatin1())).contains("BEGIN MACHINE"));}
 void offlineAndRevocation(){Store s(":memory:");seed(s);MockNetwork net;net.replies={{0,{}}};Activation a(s,nullptr,c,&net);QVERIFY(a.allowed());a.tick();QTRY_COMPARE(net.urls.size(),1);QTest::qWait(20);QVERIFY(a.allowed());net.replies={{200,validation(false,"SUSPENDED")}};a.activate("ABCD2345EFGH6789");QTRY_VERIFY(!a.allowed());Activation restarted(s,nullptr,c,&net);QVERIFY(!restarted.allowed());}
 void forgedCheckoutDenied(){Store s(":memory:");MockNetwork net;net.replies={{200,validation(true)},{200,QJsonObject{}}};Activation a(s,nullptr,c,&net);a.activate("ABCD2345EFGH6789");QTRY_COMPARE(net.urls.size(),2);QTest::qWait(20);QVERIFY(!a.allowed());}
};
QTEST_MAIN(LicenseTest)
#include "license_test.moc"
