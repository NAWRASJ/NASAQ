// SPDX-License-Identifier: GPL-3.0-or-later
#include "studio.h"
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <stdexcept>
#include <QEventLoop>
static QString json(const QJsonObject &o){return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));}
static void fail(const char *s){throw std::runtime_error(s);}
qint64 Studio::duration(const QString &file){
 QMediaPlayer player;QAudioOutput output;player.setAudioOutput(&output);QEventLoop loop;QTimer timer;timer.setSingleShot(true);
 QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
 auto ready=[&]{if(player.duration()>0&&player.hasAudio())loop.quit();};
 QObject::connect(&player,&QMediaPlayer::durationChanged,&loop,ready);
 QObject::connect(&player,&QMediaPlayer::hasAudioChanged,&loop,ready);
 QObject::connect(&player,&QMediaPlayer::mediaStatusChanged,&loop,[&](auto status){if(status==QMediaPlayer::InvalidMedia)loop.quit();else ready();});
 QObject::connect(&player,&QMediaPlayer::errorOccurred,&loop,&QEventLoop::quit);
 timer.start(10000);player.setSource(QUrl::fromLocalFile(file));if(player.duration()<=0||!player.hasAudio())loop.exec(QEventLoop::ExcludeUserInputEvents);
 if(player.error()!=QMediaPlayer::NoError||player.duration()<=0||!player.hasAudio())fail("Audio cannot be decoded on this installation");return player.duration();
}
Studio::Studio(Store &s,const QString &directory):db(s),root(directory){
 db.exec("CREATE TABLE IF NOT EXISTS studio_ads(id TEXT PRIMARY KEY,body TEXT NOT NULL)");
 db.exec("CREATE TABLE IF NOT EXISTS studio_rules(id TEXT PRIMARY KEY,ad TEXT NOT NULL,body TEXT NOT NULL,next INTEGER NOT NULL,cursor INTEGER NOT NULL DEFAULT 0,state TEXT NOT NULL DEFAULT 'active')");
}
QString Studio::importAudio(const QString &file){
 QFile src(file);auto ext=QFileInfo(file).suffix().toLower();
 if(!QStringList{"wav","mp3","m4a","aac","flac","ogg","opus","wma","aiff"}.contains(ext)||!src.open(QIODevice::ReadOnly)||src.size()==0||src.size()>256*1024*1024)fail("Invalid audio file (maximum 256 MB)");
 duration(file);
 QCryptographicHash hash(QCryptographicHash::Sha256);if(!hash.addData(&src))fail("Cannot read audio");src.seek(0);
 if(!QDir().mkpath(root))fail("Cannot create library");QString dest=QDir(root).filePath(QString::fromLatin1(hash.result().toHex())+"."+ext);
 if(!QFileInfo::exists(dest)){QSaveFile out(dest);if(!out.open(QIODevice::WriteOnly))fail("Cannot save audio");while(!src.atEnd()){auto b=src.read(1024*1024);if(b.isEmpty()||out.write(b)!=b.size())fail("Cannot copy audio");}if(!out.commit())fail("Cannot commit audio");}return dest;
}
void Studio::exportAudio(const QString &file,const QString &destination){
 if(QFileInfo(file).absoluteFilePath()==QFileInfo(destination).absoluteFilePath())fail("Choose a different destination");
 QFile src(file);QSaveFile out(destination);if(!src.open(QIODevice::ReadOnly)||!out.open(QIODevice::WriteOnly))fail("Cannot export audio");
 while(!src.atEnd()){auto b=src.read(1024*1024);if(b.isEmpty()||out.write(b)!=b.size())fail("Cannot export audio");}if(!out.commit())fail("Cannot commit export");
}
QString Studio::save(QJsonObject ad){if(ad["name"].toString().trimmed().isEmpty())fail("Enter announcement name");auto id=ad["id"].toString();if(id.isEmpty())id=QUuid::createUuid().toString(QUuid::WithoutBraces);ad["id"]=id;db.exec("INSERT INTO studio_ads VALUES(?,?) ON CONFLICT(id) DO UPDATE SET body=excluded.body",{id,json(ad)});return id;}
QJsonObject Studio::load(const QString &id){auto q=db.query("SELECT body FROM studio_ads WHERE id=?",{id});if(!q.next())fail("Announcement not found");return QJsonDocument::fromJson(q.value(0).toString().toUtf8()).object();}
QList<QJsonObject> Studio::library(const QString &search){QList<QJsonObject> out;auto q=db.query("SELECT body FROM studio_ads ORDER BY rowid DESC");while(q.next()){auto o=QJsonDocument::fromJson(q.value(0).toString().toUtf8()).object();if((o["name"].toString()+" "+o["category"].toString()).contains(search,Qt::CaseInsensitive))out<<o;}return out;}
void Studio::remove(const QString &id){auto q=db.query("SELECT count(*) FROM studio_rules WHERE ad=? AND state IN ('active','waiting')",{id});q.next();if(q.value(0).toInt())fail("Cancel linked schedules first");db.exec("DELETE FROM studio_ads WHERE id=?",{id});/* Immutable media retained for history and shared references. */}
QString Studio::payload(const QJsonObject &ad,const QStringList &languages,int gap){
 if(languages.isEmpty()||gap<0||gap>30)fail("Select languages and a valid gap");QJsonArray files,labels;auto variants=ad["variants"].toObject();
 for(const auto &lang:languages){auto v=variants[lang].toObject();auto f=v["file"].toString();if(!QFileInfo(f).isFile()||v["draft"].toString()!=v["published"].toString()||!v["approved"].toBool())fail("Approve audio for every selected language");files.append(f);labels.append(lang);}
 return json({{"files",files},{"languages",labels},{"gap",gap},{"name",ad["name"]},{"chime",ad["chime"].toString("default")}});
}
QDateTime Studio::nextSlot(const QJsonObject &r,const QDateTime &from,bool inclusive){
 auto first=QDate::fromString(r["start"].toString(),Qt::ISODate),last=QDate::fromString(r["end"].toString(),Qt::ISODate);auto start=QTime::fromString(r["from"].toString(),"HH:mm"),end=QTime::fromString(r["to"].toString(),"HH:mm");int interval=r["minutes"].toInt();
 if(!first.isValid()||!last.isValid()||first>last||first.daysTo(last)>366||!start.isValid()||!end.isValid()||start>end||interval<1||interval>1440||r["days"].toArray().isEmpty())fail("Invalid schedule dates, hours or weekdays");
 QTimeZone zone(r["zone"].toString().toUtf8());if(!zone.isValid())fail("Invalid timezone");auto local=from.toTimeZone(zone);auto date=qMax(first,local.date());
 for(;date<=last;date=date.addDays(1)){if(!r["days"].toArray().contains(date.dayOfWeek()))continue;for(int seconds=0;seconds<=start.secsTo(end);seconds+=interval*60){QDateTime t(date,start.addSecs(seconds),zone);if(t.isValid()&&(t>from||(inclusive&&t==from)))return t;}}
 return {};
}
QString Studio::schedule(const QString &adId,const QJsonObject &rule,const QDateTime &now){
 auto ad=load(adId);QStringList languages;for(auto l:rule["languages"].toArray())languages<<l.toString();auto p=payload(ad,languages,rule["gap"].toInt());auto next=nextSlot(rule,now);if(!next.isValid())fail("No future occurrence in this schedule");
 auto r=rule;r["snapshot"]=QJsonDocument::fromJson(p.toUtf8()).object();auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);db.exec("INSERT INTO studio_rules(id,ad,body,next) VALUES(?,?,?,?)",{id,adId,json(r),next.toMSecsSinceEpoch()});return id;
}
StudioEngine::StudioEngine(Studio &s,Audio &a,QObject *p):QObject(p),studio(s),audio(a){
 studio.db.exec("UPDATE studio_rules SET state='active' WHERE state='waiting'");
 connect(&audio,&Audio::sequenceFinished,this,[this](const QString &id,const QString &status){
  auto q=studio.db.query("SELECT body FROM studio_rules WHERE id=? AND state='waiting'",{id});if(!q.next())return;auto r=QJsonDocument::fromJson(q.value(0).toString().toUtf8()).object();auto from=QDateTime::currentDateTimeUtc().addSecs(r["minutes"].toInt()*60);auto next=Studio::nextSlot(r,from);
  // After-completion cadence can start between fixed grid positions inside today's window.
  QTimeZone z(r["zone"].toString().toUtf8());auto local=from.toTimeZone(z);if(local.date()>=QDate::fromString(r["start"].toString(),Qt::ISODate)&&local.date()<=QDate::fromString(r["end"].toString(),Qt::ISODate)&&r["days"].toArray().contains(local.date().dayOfWeek())&&local.time()>=QTime::fromString(r["from"].toString(),"HH:mm")&&local.time()<=QTime::fromString(r["to"].toString(),"HH:mm"))next=local;
  studio.db.exec("UPDATE studio_rules SET state=?,next=? WHERE id=?",{next.isValid()?"active":"done",next.toMSecsSinceEpoch(),id});studio.db.log("studio_completion",id+":"+status);
 });
}
void StudioEngine::tick(const QDateTime &now){
 auto q=studio.db.query("SELECT id,body,next,cursor FROM studio_rules WHERE state='active' AND next<=? ORDER BY next,id",{now.toMSecsSinceEpoch()});
 while(q.next()){auto id=q.value(0).toString();auto r=QJsonDocument::fromJson(q.value(1).toString().toUtf8()).object();auto due=QDateTime::fromMSecsSinceEpoch(q.value(2).toLongLong(),QTimeZone::UTC);int cursor=q.value(3).toInt();bool stale=due.secsTo(now)>60,busy=audio.activePriority()>0;
  if(!stale&&busy&&r["wait"].toBool())continue;
  auto next=Studio::nextSlot(r,now,false);bool dispatch=!stale&&!busy;
  studio.db.exec("UPDATE studio_rules SET next=?,cursor=cursor+?,state=? WHERE id=?",{next.toMSecsSinceEpoch(),dispatch?1:0,dispatch&&r["after"].toBool()?"waiting":next.isValid()?"active":"done",id});
  if(!dispatch){studio.db.log("studio_skipped",id+(stale?":missed":":busy"));continue;}
  auto p=r["snapshot"].toObject();if(r["rotate"].toBool()){auto files=p["files"].toArray(),langs=p["languages"].toArray();if(files.isEmpty())continue;int i=cursor%files.size();p["files"]=QJsonArray{files[i]};p["languages"]=QJsonArray{langs[i]};}
  if(!audio.dispatch({id,"sequence",json(p),now,50})){studio.db.exec("UPDATE studio_rules SET state=? WHERE id=?",{next.isValid()?"active":"done",id});studio.db.log("studio_rejected",id);}else studio.db.log("studio_dispatched",id);
 }
}
