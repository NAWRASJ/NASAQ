// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtSql>
#include <QJsonObject>
struct Event { QString id, kind, payload; QDateTime due; int priority=10; };
class Store {
 public:
 explicit Store(const QString &path);
 ~Store();
 void exec(const QString &sql, const QVariantList &args={});
 QSqlQuery query(const QString &sql, const QVariantList &args={});
 void log(const QString &kind, const QString &detail);
 void add(const Event &event);
 QList<Event> due(const QDateTime &now);
 bool claim(const QString &id);
 void finish(const QString &id, const QString &status);
 void set(const QString &key, const QString &value);
 QString get(const QString &key);
 static QString csvCell(QString value);
 void exportCsv(const QString &path);
 private: QSqlDatabase db; QString connection;
};
