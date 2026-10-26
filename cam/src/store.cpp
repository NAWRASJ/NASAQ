// SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include <QSaveFile>
#include <stdexcept>
Store::Store(const QString &path):connection(QUuid::createUuid().toString()) {
 db=QSqlDatabase::addDatabase("QSQLITE",connection); db.setDatabaseName(path);
 if(!db.open()) throw std::runtime_error(db.lastError().text().toStdString());
 exec("PRAGMA journal_mode=WAL"); exec("PRAGMA synchronous=FULL"); exec("PRAGMA busy_timeout=5000");
 exec("CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY,value TEXT NOT NULL)");
 exec("CREATE TABLE IF NOT EXISTS events(id TEXT PRIMARY KEY,kind TEXT NOT NULL,payload TEXT NOT NULL,due INTEGER NOT NULL,priority INTEGER NOT NULL,status TEXT NOT NULL DEFAULT 'pending')");
 exec("CREATE TABLE IF NOT EXISTS logs(id INTEGER PRIMARY KEY,utc TEXT NOT NULL,kind TEXT NOT NULL,detail TEXT NOT NULL)");
 exec("CREATE TABLE IF NOT EXISTS licenses(id TEXT PRIMARY KEY,issuer TEXT,reference TEXT,expires TEXT,document TEXT)");
 // At-most-once dispatch: never automatically replay a claimed announcement after a crash.
 exec("UPDATE events SET status='interrupted' WHERE status='claimed'");
}
Store::~Store(){db.close();db=QSqlDatabase();QSqlDatabase::removeDatabase(connection);}
QSqlQuery Store::query(const QString &sql,const QVariantList &args){ QSqlQuery q(db); if(!q.prepare(sql))throw std::runtime_error(q.lastError().text().toStdString());for(const auto &v:args)q.addBindValue(v);if(!q.exec())throw std::runtime_error(q.lastError().text().toStdString());return q; }
void Store::exec(const QString&s,const QVariantList&a){query(s,a);}
void Store::log(const QString&k,const QString&d){exec("INSERT INTO logs(utc,kind,detail) VALUES(?,?,?)",{QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),k,d});}
void Store::add(const Event&e){if(!e.due.isValid()||e.id.isEmpty())throw std::runtime_error("Invalid event");exec("INSERT INTO events(id,kind,payload,due,priority) VALUES(?,?,?,?,?)",{e.id,e.kind,e.payload.isNull()?QStringLiteral(""):e.payload,e.due.toMSecsSinceEpoch(),e.priority});}
QList<Event> Store::due(const QDateTime&now){
 // Keep the original timetable intact. A separately claimed silent hold starts
 // early, while any adhan audio remains scheduled at its original time.
 const int lead=get("prayer_pause_enabled")=="0"?0:qBound(0,get("prayer_lead_minutes").toInt(),5);
 if(lead>0)exec("INSERT OR IGNORE INTO events(id,kind,payload,due,priority) SELECT 'prayer-early:'||id,'prayer','',due,60 FROM events WHERE kind IN ('prayer','adhan') AND id NOT LIKE 'prayer-early:%' AND status='pending' AND due>? AND due<=?",{now.toMSecsSinceEpoch(),now.addSecs(lead*60).toMSecsSinceEpoch()});
 // After sleep/restart, retain prayer events while their pause window is still
 // active. Announcements remain at-most-once and are not replayed late.
 int minutes=get("prayer_pause_minutes").isEmpty()?10:qBound(1,get("prayer_pause_minutes").toInt(),60);
 exec("UPDATE events SET status='missed' WHERE status='pending' AND ((kind NOT IN ('prayer','adhan') AND due < ?) OR (kind IN ('prayer','adhan') AND due <= ?))",{now.addSecs(-60).toMSecsSinceEpoch(),now.addSecs(-minutes*60).toMSecsSinceEpoch()});
 auto q=query("SELECT id,kind,payload,due,priority FROM events WHERE status='pending' AND (due<=? OR (id LIKE 'prayer-early:%' AND due<=?)) ORDER BY priority DESC,due,id",{now.toMSecsSinceEpoch(),now.addSecs(lead*60).toMSecsSinceEpoch()}); QList<Event> out;
 while(q.next()){
  auto time=QDateTime::fromMSecsSinceEpoch(q.value(3).toLongLong(),QTimeZone::UTC);auto kind=q.value(1).toString();
  if(kind=="adhan"&&time.secsTo(now)>60)kind="prayer"; // silent catch-up, never a late adhan
  out.append({q.value(0).toString(),kind,q.value(2).toString(),time,q.value(4).toInt()});
 }return out;
}
bool Store::claim(const QString&id){auto q=query("UPDATE events SET status='claimed' WHERE id=? AND status='pending'",{id});return q.numRowsAffected()==1;}
void Store::finish(const QString&id,const QString&s){exec("UPDATE events SET status=? WHERE id=?",{s,id});}
void Store::set(const QString&k,const QString&v){exec("INSERT INTO settings VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",{k,v.isNull()?QStringLiteral(""):v});}
QString Store::get(const QString&k){auto q=query("SELECT value FROM settings WHERE key=?",{k});return q.next()?q.value(0).toString():QString();}
QString Store::csvCell(QString v){if(!v.isEmpty() && QString("=+-@\t\r\n").contains(v.at(0)))v.prepend('\'');v.replace('"',"\"\"");return '"'+v+'"';}
void Store::exportCsv(const QString&path){QSaveFile f(path);if(!f.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot write CSV");f.write("\xEF\xBB\xBFutc,kind,detail\r\n");auto q=query("SELECT utc,kind,detail FROM logs ORDER BY id");while(q.next()){QStringList row;for(int i=0;i<3;i++)row<<csvCell(q.value(i).toString());f.write((row.join(',')+"\r\n").toUtf8());}if(!f.commit())throw std::runtime_error("Cannot commit CSV");}
