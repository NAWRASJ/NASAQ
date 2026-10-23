// SPDX-License-Identifier: GPL-3.0-or-later
#include "prayers.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QTimer>
#include <QFileInfo>
#include <QCryptographicHash>
#include <stdexcept>
PrayerService::PrayerService(Store &s,QObject *parent):QObject(parent),store(s){
 auto timer=new QTimer(this);connect(timer,&QTimer::timeout,this,&PrayerService::refreshSaved);timer->start(3600000);
 QTimer::singleShot(5000,this,&PrayerService::refreshSaved);
}
QDateTime PrayerService::nextPause(Store &s,const QDateTime &now){
 if(s.get("prayer_pause_enabled")=="0")return {};
 auto q=s.query("SELECT due FROM events WHERE kind IN ('prayer','adhan') AND status='pending' AND due>=? ORDER BY due LIMIT 1",{now.toMSecsSinceEpoch()});
 return q.next()?QDateTime::fromMSecsSinceEpoch(q.value(0).toLongLong(),QTimeZone::UTC):QDateTime();
}
void PrayerService::refreshSaved(){
 try{if(store.get("prayer_enabled")=="1"&&store.get("prayer_sync_date")!=QDate::currentDate().toString(Qt::ISODate))sync(store.get("prayer_city"),store.get("prayer_country"),store.get("prayer_method").toInt(),store.get("prayer_audio"),true);}catch(const std::exception&e){Q_EMIT completed(false,QString::fromUtf8(e.what()));}
}
QList<Event> PrayerService::parseCalendar(const QByteArray &json,const QString &identity,const QString &audio,const QDateTime &now){
 QJsonParseError error;auto doc=QJsonDocument::fromJson(json,&error);auto root=doc.object();
 if(error.error!=QJsonParseError::NoError||root["code"].toInt()!=200||!root["data"].isArray()||root["data"].toArray().isEmpty())throw std::runtime_error("Invalid prayer service response");
 QList<Event> events;QSet<QString> ids;
 for(const auto &day:root["data"].toArray()){
  const auto row=day.toObject();const auto timings=row["timings"].toObject();
  for(const auto &name:QStringList{"Fajr","Dhuhr","Asr","Maghrib","Isha"}){
   auto value=timings[name].toString();auto time=QDateTime::fromString(value,Qt::ISODate);
   // Never interpret a provider's bare local time using this PC's timezone.
   if(!time.isValid()||!QRegularExpression("(Z|[+-][0-9]{2}:[0-9]{2})$").match(value).hasMatch())throw std::runtime_error("Prayer time is missing an explicit timezone offset");
   auto id="prayer-api:"+identity+":"+time.date().toString(Qt::ISODate)+":"+name;
   if(ids.contains(id))throw std::runtime_error("Duplicate prayer time");ids.insert(id);
   if(time>now)events.append({id,audio.isEmpty()?"prayer":"adhan",audio,time.toUTC(),60});
  }
 }
 return events;
}
void PrayerService::saveCalendar(Store&s,const QList<Event>&events){
 s.exec("BEGIN IMMEDIATE");try{
  // Replace only service-generated future pending rows. Preserve manual schedules and history.
  s.exec("DELETE FROM events WHERE id LIKE 'prayer-api:%' AND status='pending'");
  for(const auto&e:events){auto q=s.query("SELECT id FROM events WHERE id=?",{e.id});if(!q.next())s.add(e);}
  s.exec("COMMIT");
 }catch(...){s.exec("ROLLBACK");throw;}
}
void PrayerService::sync(const QString &city,const QString &country,int method,const QString &adhan,bool enabled){
 if(busy){Q_EMIT completed(false,QStringLiteral("Prayer sync already running"));return;}
 if(!enabled){store.set("prayer_enabled","0");saveCalendar(store,{});Q_EMIT completed(true,QStringLiteral("Automatic prayer schedule disabled"));return;}
 if(city.trimmed().isEmpty()||country.trimmed().isEmpty()||(!adhan.isEmpty()&&!QFileInfo(adhan).isFile())){Q_EMIT completed(false,QStringLiteral("Choose a city, country and an existing adhan file"));return;}
 busy=true;
 auto date=QDate::currentDate();QUrl url(QString("https://api.aladhan.com/v1/calendarByCity/%1/%2").arg(date.year()).arg(date.month()));
 QUrlQuery query;query.addQueryItem("city",city.trimmed());query.addQueryItem("country",country.trimmed());query.addQueryItem("method",QString::number(method));query.addQueryItem("iso8601","true");url.setQuery(query);
 QNetworkRequest req(url);req.setTransferTimeout(15000);req.setRawHeader("User-Agent","NASAQ/0.3.10");auto reply=network.get(req);
 connect(reply,&QNetworkReply::finished,this,[this,reply,city,country,method,adhan,date]{
  busy=false;reply->deleteLater();
  try{
   if(reply->error()!=QNetworkReply::NoError)throw std::runtime_error(reply->errorString().toStdString());
   auto bytes=reply->readAll();if(bytes.size()>2*1024*1024)throw std::runtime_error("Prayer response too large");
   auto identity=QString::fromLatin1(QCryptographicHash::hash((city+"|"+country+"|"+QString::number(method)).toUtf8(),QCryptographicHash::Sha256).toHex().left(16));
   auto rows=parseCalendar(bytes,identity,adhan,QDateTime::currentDateTimeUtc());if(rows.isEmpty())throw std::runtime_error("No future prayer times received");
   saveCalendar(store,rows);store.set("prayer_timezone",QJsonDocument::fromJson(bytes).object()["data"].toArray().first().toObject()["meta"].toObject()["timezone"].toString());store.set("prayer_city",city);store.set("prayer_country",country);store.set("prayer_method",QString::number(method));store.set("prayer_audio",adhan);store.set("prayer_enabled","1");store.set("prayer_sync_date",date.toString(Qt::ISODate));
   store.log("prayer_sync",city+", "+country+"; AlAdhan; method="+QString::number(method));Q_EMIT completed(true,QStringLiteral("Prayer schedule saved: ")+QString::number(rows.size()));
  }catch(const std::exception&e){Q_EMIT completed(false,QString::fromUtf8(e.what()));}
 });
}
