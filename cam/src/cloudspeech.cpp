// SPDX-License-Identifier: GPL-3.0-or-later
#include "cloudspeech.h"
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QtEndian>
#include <QTimer>
#include <stdexcept>

CloudSpeech::CloudSpeech(const QString &dir,QObject *parent,QNetworkAccessManager *manager):QObject(parent),directory(dir),network(manager?manager:new QNetworkAccessManager(this)){}
CloudSpeech::~CloudSpeech(){if(reply){disconnect(reply,nullptr,this,nullptr);reply->abort();reply->deleteLater();}}
QJsonObject CloudSpeech::requestBody(const QString &text,const QString &language,const QString &voice){
 if(text.trimmed().isEmpty() || text.toUtf8().size()>5000) throw std::runtime_error("TextLimit");
 if(!QStringList{"ar-XA","en-US","en-GB","hi-IN"}.contains(language)) throw std::runtime_error("VoiceInvalid");
 if(!QRegularExpression("^"+QRegularExpression::escape(language)+"-Chirp3-HD-[A-Za-z]+$").match(voice).hasMatch()) throw std::runtime_error("VoiceInvalid");
 return {{"input",QJsonObject{{"text",text}}},{"voice",QJsonObject{{"languageCode",language},{"name",voice}}},{"audioConfig",QJsonObject{{"audioEncoding","LINEAR16"},{"sampleRateHertz",24000}}}};
}
bool CloudSpeech::validWave(const QByteArray &data){
 if(data.size()<44 || data.size()>12*1024*1024 || data.first(4)!="RIFF" || data.mid(8,4)!="WAVE") return false;
 auto u32=[&](qsizetype p){return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()+p));};
 if(quint64(u32(4))+8!=quint64(data.size())) return false;
 bool fmt=false,audio=false;
 for(qsizetype p=12;p+8<=data.size();){quint32 n=u32(p+4);if(quint64(p)+8+n>quint64(data.size()))return false;
  if(data.mid(p,4)=="fmt "){if(n<16)return false;auto u16=[&](qsizetype x){return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(data.constData()+x));};fmt=u16(p+8)==1 && u16(p+10)==1 && u32(p+12)==24000 && u16(p+20)==2 && u16(p+22)==16;}
  if(data.mid(p,4)=="data")audio=n>0 && n%2==0;
  p+=8+qsizetype(n)+(n%2);
 }
 return fmt&&audio;
}
QByteArray CloudSpeech::decodeAudio(const QByteArray &json){
 QJsonParseError error;auto doc=QJsonDocument::fromJson(json,&error);
 if(error.error!=QJsonParseError::NoError || !doc.isObject())throw std::runtime_error("InvalidAudio");
 auto decoded=QByteArray::fromBase64Encoding(doc.object().value("audioContent").toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
 if(!decoded || !validWave(decoded.decoded))throw std::runtime_error("InvalidAudio");
 return decoded.decoded;
}
void CloudSpeech::cancel(){if(reply){reply->setProperty("cancelled",true);reply->abort();}}
void CloudSpeech::generate(const QString &text,const QString &language,const QString &voice,const QString &project,const QString &token){
 if(busy()){emit failed("Busy");return;}
 QByteArray body;
 try{body=QJsonDocument(requestBody(text,language,voice)).toJson(QJsonDocument::Compact);}catch(const std::exception &e){emit failed(QString::fromUtf8(e.what()));return;}
 const auto file=QDir(directory).filePath(QString::fromLatin1(QCryptographicHash::hash(body,QCryptographicHash::Sha256).toHex())+".wav");
 QFile cached(file);if(cached.open(QIODevice::ReadOnly)&&cached.size()<=12*1024*1024&&validWave(cached.readAll())){emit ready(file,true);return;}cached.close();
 if(!QRegularExpression("^[a-z][a-z0-9-]{4,61}[a-z0-9]$").match(project).hasMatch()){emit failed("ProjectInvalid");return;}
 if(token.isEmpty() || token.size()>8192 || token.contains(QRegularExpression("[\\s\\x00-\\x1f\\x7f]"))){emit failed("TokenRequired");return;}
 if(!QDir().mkpath(directory)){emit failed("SaveFailed");return;}
 // A conservative, per-project per-installation cap. Reservations are retained
 // after failures, because the provider may have processed a timed-out request.
 QSettings settings;QString quota="cloudSpeech/usage/"+project+"/"+QDate::currentDate().toString("yyyy-MM");
 int used=settings.value(quota,0).toInt(),count=text.toUcs4().size();
 if(used<0 || used>100000-count){emit failed("LocalLimit");return;}
 settings.setValue(quota,used+count);settings.sync();if(settings.status()!=QSettings::NoError){emit failed("SaveFailed");return;}
 QNetworkRequest request(QUrl("https://texttospeech.googleapis.com/v1/text:synthesize"));
 request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
 request.setRawHeader("Authorization","Bearer "+token.toUtf8());request.setRawHeader("x-goog-user-project",project.toUtf8());
 request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);request.setTransferTimeout(60000);
 auto current=network->post(request,body);reply=current;current->setReadBufferSize(16*1024*1024);
 QTimer::singleShot(60000,current,[current]{if(!current->isFinished())current->abort();});
 connect(current,&QNetworkReply::readyRead,this,[current]{QByteArray buffer=current->property("body").toByteArray();buffer+=current->readAll();if(buffer.size()>16*1024*1024){current->setProperty("tooLarge",true);current->abort();return;}current->setProperty("body",buffer);});
 connect(current,&QNetworkReply::finished,this,[this,current,file]{
  reply.clear();current->deleteLater();
  if(current->property("cancelled").toBool()){emit failed("Cancelled");return;}
  if(current->property("tooLarge").toBool()){emit failed("InvalidAudio");return;}
  int status=current->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if(status!=200 || current->error()!=QNetworkReply::NoError){emit failed(status==401?"Authentication":status==403?"Permission":status==429?"Quota":"Network");return;}
  try{auto data=decodeAudio(current->property("body").toByteArray()+current->readAll());QSaveFile output(file);if(!output.open(QIODevice::WriteOnly)||output.write(data)!=data.size()||!output.commit()){emit failed("SaveFailed");return;}emit ready(file,false);}catch(...){emit failed("InvalidAudio");}
 });
}
