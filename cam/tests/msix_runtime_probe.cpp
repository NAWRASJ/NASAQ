// SPDX-License-Identifier: GPL-3.0-or-later
// Run with QT_PLUGIN_PATH=<clean stage> and a clean PATH. Does not alter real profiles.
#include "packageidentity.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 app.setOrganizationName(QStringLiteral("NASAQTests"));
 app.setApplicationName(QStringLiteral("MSIXRuntimeProbe"));
 QTemporaryDir temporary;
 const auto formats=QImageReader::supportedImageFormats();
 auto database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
 database.setDatabaseName(temporary.filePath(QStringLiteral("probe.sqlite")));
 const bool sql=database.open() && QSqlQuery(database).exec(QStringLiteral("CREATE TABLE probe (id INTEGER)"));
 QJsonArray images;for(const auto &format:formats)images.append(QString::fromLatin1(format));
 QJsonObject result{{"packaged",Nasaq::isPackaged()},{"storeManaged",Nasaq::storeManaged()},
  {"platform",QApplication::platformName()},{"sqliteWrite",sql},{"imageFormats",images},
  {"pluginPaths",QJsonArray::fromStringList(QCoreApplication::libraryPaths())},
  {"dataPath",QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)},
  {"cachePath",QStandardPaths::writableLocation(QStandardPaths::CacheLocation)}};
 const auto output=QJsonDocument(result).toJson();std::fwrite(output.constData(),1,output.size(),stdout);
 return sql && formats.contains("jpeg") && formats.contains("png") ? 0 : 1;
}
