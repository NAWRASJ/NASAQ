// SPDX-License-Identifier: GPL-3.0-or-later
#include "datamanager.h"
#include "licenselease.h"
#include "nasaqlanguage.h"
#include "silentmessagebox.h"
#include "store.h"
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <stdexcept>
#include <QScopeGuard>
namespace NasaqData {
namespace {
constexpr qint64 MaxBytes=128*1024*1024;
const QByteArray Magic("NASAQ-BACKUP-1\n");
const QString Pending=QStringLiteral(".nasaq-maintenance.json");
void require(bool ok,const char *message){if(!ok){qWarning("NASAQ data management: %s",message);throw std::runtime_error(message);}}
QString text(const char *en,const char *ar){return QString::fromUtf8(NasaqLanguage()=="ar"?ar:en);}
QString hash(const QByteArray &b){return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Cannot read backup data");require(f.size()<=MaxBytes,"Backup exceeds the 128 MB safety limit");return f.readAll();}
void write(const QString &path,const QByteArray &bytes){QSaveFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size()&&f.commit(),"Cannot safely save backup data");}
bool safeName(const QString &name){
 if(name.isEmpty()||name.size()>512||name.contains('\\')||name.contains(':')||name.startsWith('/')||QDir::cleanPath(name)!=name)return false;
 for(const auto &part:name.split('/'))if(part==".."||part=="."||part.isEmpty()||part.endsWith('.')||part.endsWith(' ')||part.startsWith(".nasaq-")||part.contains(QRegularExpression("[<>\"|?*\\x00-\\x1f]"))||QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\\..*)?$",QRegularExpression::CaseInsensitiveOption).match(part).hasMatch())return false;
 return true;
}
QJsonObject settings(QSettings &s){s.sync();require(s.status()==QSettings::NoError,"Cannot read application settings");QJsonObject result;
 for(const auto &key:s.allKeys()){QByteArray value;QDataStream stream(&value,QIODevice::WriteOnly);stream.setVersion(QDataStream::Qt_6_0);stream<<s.value(key);result[key]=QString::fromLatin1(value.toBase64());}return result;}
void setSettings(QSettings &s,const QJsonObject &values){s.clear();for(auto i=values.begin();i!=values.end();++i){QByteArray bytes=QByteArray::fromBase64(i.value().toString().toLatin1());QDataStream stream(bytes);stream.setVersion(QDataStream::Qt_6_0);QVariant v;stream>>v;require(stream.status()==QDataStream::Ok,"Invalid settings in backup");s.setValue(i.key(),v);}s.sync();require(s.status()==QSettings::NoError,"Could not restore settings");}
QByteArray snapshot(const QString &path){
 auto bytes=read(path);if(!bytes.startsWith("SQLite format 3"))return bytes;
 QTemporaryDir temp;require(temp.isValid(),"Cannot create database snapshot directory");const auto output=temp.filePath("snapshot.sqlite");const auto connection=QUuid::createUuid().toString();bool ok=false;
 {auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(path);db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=5000");
 if(db.open()){QSqlQuery q(db);QString escaped=output;escaped.replace('\'',"''");ok=q.exec("VACUUM INTO '"+escaped+"'");}db.close();}
 QSqlDatabase::removeDatabase(connection);require(ok,"Cannot create a consistent database snapshot; stop playback and try again");return read(output);
}
QJsonObject unpack(const QString &path){auto data=read(path);require(data.startsWith(Magic),"This is not a NASAQ backup");auto plain=unprotectLicenseData(data.mid(Magic.size()));require(!plain.isEmpty(),"Backup is damaged or belongs to another Windows account/device");
 QJsonParseError error;auto doc=QJsonDocument::fromJson(plain,&error);require(error.error==QJsonParseError::NoError&&doc.isObject(),"Invalid backup document");auto o=doc.object();require(o["format"].toInt()==1&&o["product"].toString()=="NASAQ"&&o["files"].isArray(),"Unsupported backup format");
 require(o["files"].toArray().size()<=10000&&o["settings"].isObject()&&o["preferences"].isObject(),"Invalid backup contents");QSet<QString> names;qint64 total=0;for(const auto &entry:o["files"].toArray()){const auto f=entry.toObject();auto name=f["name"].toString();require(safeName(name)&&!names.contains(name.toCaseFolded()),"Unsafe or duplicate backup path");names.insert(name.toCaseFolded());auto b=QByteArray::fromBase64(f["data"].toString().toLatin1());total+=b.size();require(total<=MaxBytes&&hash(b)==f["sha256"].toString(),"Backup integrity check failed");}
 require(names.contains("cam/cam.sqlite")&&names.contains("strawberry.db"),"Backup is missing NASAQ databases");return o;
}
void pending(const QString &root,QJsonObject o){require(QFileInfo(root).isDir()&&!QFileInfo(root).isSymLink()&&!QFileInfo(root).isJunction()&&QFileInfo::exists(root+"/CAM/cam.sqlite")&&QFileInfo::exists(root+"/strawberry.db"),"Invalid profile directory");require(!QFileInfo::exists(root+"/"+Pending),"A maintenance operation is already pending");write(root+"/"+Pending,QJsonDocument(o).toJson());}
void checkDatabase(const QString &path){const auto id=QUuid::createUuid().toString();bool ok=false;{auto db=QSqlDatabase::addDatabase("QSQLITE",id);db.setDatabaseName(path);if(db.open()){QSqlQuery q(db);ok=q.exec("PRAGMA quick_check")&&q.next()&&q.value(0).toString()=="ok";}db.close();}QSqlDatabase::removeDatabase(id);require(ok,"Restored database integrity check failed");}
}
QString profilePath(){return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);}
void backup(const QString &root,const QString &destination){
 const auto canonical=QFileInfo(root).canonicalFilePath();require(!canonical.isEmpty()&&!QFileInfo(root).isSymLink()&&!QFileInfo(root).isJunction(),"Invalid profile directory");
 const auto parent=QFileInfo(QFileInfo(destination).absolutePath()).canonicalFilePath();require(!parent.isEmpty(),"Backup destination folder does not exist");const auto dest=parent+"/"+QFileInfo(destination).fileName();require(!dest.startsWith(canonical+"/",Qt::CaseInsensitive),"Save the backup outside NASAQ's data directory");
 QJsonArray files;qint64 total=0;QDirIterator it(root,QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden,QDirIterator::Subdirectories);
 while(it.hasNext()){const auto path=it.next();auto info=it.fileInfo();require(!info.isSymLink()&&!info.isJunction(),"Symbolic links and junctions are not allowed in a profile backup");if(info.isDir())continue;auto name=QDir(root).relativeFilePath(path);if(name.startsWith("gst-registry-")||name.endsWith(".lock")||name.endsWith("-wal")||name.endsWith("-shm")||name==Pending)continue;require(safeName(name),"Unsafe profile path");auto bytes=snapshot(path);total+=bytes.size();require(total<=MaxBytes/2,"Profile is too large for this backup; archive large generated media separately");require(files.size()<10000,"Too many files in profile backup");files.append(QJsonObject{{"name",name},{"sha256",hash(bytes)},{"data",QString::fromLatin1(bytes.toBase64())}});}
 QSettings main;QSettings preferences(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences");QJsonObject o{{"format",1},{"product","NASAQ"},{"version",QCoreApplication::applicationVersion()},{"createdUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"originalRoot",canonical},{"files",files},{"settings",settings(main)},{"preferences",settings(preferences)}};
 auto sealed=protectLicenseData(QJsonDocument(o).toJson(QJsonDocument::Compact));require(!sealed.isEmpty(),"Windows could not encrypt the backup");require(sealed.size()<MaxBytes,"Backup exceeds the safety limit");write(destination,Magic+sealed);validateBackup(destination);
}
void validateBackup(const QString &path){unpack(path);}
void queueRestore(const QString &root,const QString &path,bool keepLicense){auto archive=unpack(path);require(archive["originalRoot"].toString().compare(QFileInfo(root).canonicalFilePath(),Qt::CaseInsensitive)==0,"Restore requires the original profile path on this device");pending(root,{{"action","restore"},{"path",QFileInfo(path).absoluteFilePath()},{"sha256",hash(read(path))},{"keepLicense",keepLicense}});}
void queueReset(const QString &root,bool keepLicense){pending(root,{{"action","reset"},{"keepLicense",keepLicense}});}
// The encrypted journal permits rollback if Windows or the app stops between
// directory replacement and settings restoration. Never roll back after cleanup starts.
bool applyPending(const QString &root){
 const auto request=root+"/"+Pending;
 const auto journal=root+".nasaq-transaction";
 const auto stage=root+".nasaq-stage";
 const auto previous=root+".nasaq-previous";
 if(!QFileInfo::exists(request)&&!QFileInfo::exists(journal))return false;
 require(QDir::isAbsolutePath(root)&&!QFileInfo(root).fileName().isEmpty()&&!QFileInfo(root).isSymLink()&&!QFileInfo(root).isJunction(),"Invalid maintenance profile path");
 for(const auto &path:{stage,previous,journal})require(!QFileInfo(path).isSymLink()&&!QFileInfo(path).isJunction(),"Unsafe maintenance path");
 QSettings main;QSettings preferences(QSettings::defaultFormat(),QSettings::UserScope,"NASAQ","Preferences");
 auto recover=[&]{
  auto plain=unprotectLicenseData(read(journal));auto j=QJsonDocument::fromJson(plain).object();
  require(j["profile"].toString()==root&&j["settings"].isObject()&&j["preferences"].isObject(),"Cannot verify the recovery journal; original files have been retained");
  if(!j["committed"].toBool()){
   if(QFileInfo::exists(previous)){
    if(QFileInfo::exists(root)){require(!QFileInfo::exists(stage),"Recovery staging path is occupied");require(QDir().rename(root,stage),"Cannot move incomplete restore aside");}
    require(QDir().rename(previous,root),"Cannot recover original profile; close other NASAQ instances");
   }
   setSettings(main,j["settings"].toObject());setSettings(preferences,j["preferences"].toObject());
   QFile::remove(request);
  }
  // Cleanup is retryable. A failure here must never restore a partially deleted directory.
  require(!QFileInfo::exists(stage)||QDir(stage).removeRecursively(),"Data is safe, but temporary data cleanup failed; close other NASAQ instances and restart");
  require(!QFileInfo::exists(previous)||QDir(previous).removeRecursively(),"Data is safe, but old data cleanup failed; close other NASAQ instances and restart");
  require(QFile::remove(journal),"Cannot finish maintenance journal cleanup");
 };
 if(QFileInfo::exists(journal)){recover();return true;}
 // Invalid/cancelled operations cannot trap the user in a repeated startup failure.
 auto cancel=qScopeGuard([&]{QFile::remove(request);});
 auto o=QJsonDocument::fromJson(read(request)).object();
 require(o["action"]=="restore"||o["action"]=="reset","Invalid maintenance operation");
 require(QFileInfo::exists(root+"/CAM/cam.sqlite")&&QFileInfo::exists(root+"/strawberry.db"),"Profile databases are missing; maintenance cancelled");
 require(!QFileInfo::exists(stage)&&!QFileInfo::exists(previous),"Maintenance folders already exist; original files have been retained");
 QJsonObject archive;
 if(o["action"]=="restore"){
  require(hash(read(o["path"].toString()))==o["sha256"].toString(),"Backup changed after selection");archive=unpack(o["path"].toString());
  require(archive["originalRoot"].toString().compare(QFileInfo(root).canonicalFilePath(),Qt::CaseInsensitive)==0,"Backup belongs to a different profile path");
 }
 QString license;
 if(o["keepLicense"].toBool(true)){Store current(root+"/CAM/cam.sqlite");license=current.get("activation_sealed_v1");}
 QJsonObject j{{"profile",root},{"settings",settings(main)},{"preferences",settings(preferences)},{"committed",false}};
 auto saveJournal=[&]{auto sealed=protectLicenseData(QJsonDocument(j).toJson(QJsonDocument::Compact));require(!sealed.isEmpty(),"Cannot encrypt recovery journal");write(journal,sealed);};
 saveJournal();
 try{
  require(QDir().mkpath(stage+"/CAM"),"Cannot stage restored data");
  if(o["action"]=="restore")for(const auto &entry:archive["files"].toArray()){
   auto f=entry.toObject();auto target=stage+"/"+f["name"].toString();
   require(QDir().mkpath(QFileInfo(target).absolutePath()),"Cannot create restore directory");
   write(target,QByteArray::fromBase64(f["data"].toString().toLatin1()));
  }
  {Store next(stage+"/CAM/cam.sqlite");if(o["keepLicense"].toBool(true)&&!license.isEmpty())next.set("activation_sealed_v1",license);}
  checkDatabase(stage+"/CAM/cam.sqlite");if(o["action"]=="restore")checkDatabase(stage+"/strawberry.db");
  require(QDir().rename(root,previous),"Cannot replace profile; close other NASAQ instances");
  require(QDir().rename(stage,root),"Cannot activate restored profile");
  setSettings(main,archive["settings"].toObject());setSettings(preferences,archive["preferences"].toObject());
  j["committed"]=true;saveJournal();
 }catch(...){recover();throw;}
 recover();return true;
}
void showDialog(QWidget *parent){
 QDialog d(parent);d.setObjectName("dataManagementDialog");d.setWindowTitle(text("Data and license management","إدارة البيانات والترخيص"));d.setMinimumWidth(620);auto layout=new QVBoxLayout(&d);
 auto help=new QLabel(text("Before uninstalling, save an encrypted backup outside the application folders. It includes settings, playlists, schedules, imported assets and the saved license. Restore requires the same Windows account on this device; license validity is checked normally. Music/video files referenced outside NASAQ are not copied. Backups may contain cloud credentials: keep them private. Windows controls Store app removal; uninstalling does not cancel your purchase or free a license device slot.","قبل إزالة نسق، احفظ نسخة احتياطية مشفرة خارج مجلدات التطبيق. تشمل الإعدادات والقوائم والجداول والملفات المستوردة والترخيص المحفوظ. تتطلب الاستعادة حساب Windows نفسه على هذا الجهاز، ويُتحقق من صلاحية الترخيص كالمعتاد. لا تُنسخ ملفات الموسيقى والفيديو الموجودة خارج نسق. قد تتضمن النسخة بيانات وصول للخدمات السحابية؛ احتفظ بها بشكل خاص. يدير Windows إزالة نسخة المتجر؛ الإزالة لا تلغي شراء الترخيص ولا تحرر حصة الجهاز."));help->setWordWrap(true);layout->addWidget(help);
 auto run=[&](const std::function<void()> &fn){try{fn();}catch(const std::exception &e){SilentMessageBox::warning(&d,text("Operation failed","تعذر تنفيذ العملية"),QString::fromUtf8(e.what()));}};
 auto exportBackup=new QPushButton(text("Save encrypted backup…","حفظ نسخة احتياطية مشفرة…"));exportBackup->setObjectName("exportDataBackup");layout->addWidget(exportBackup);
 auto chooseBackup=[&](){return QFileDialog::getSaveFileName(&d,text("Save backup outside app folders","حفظ النسخة خارج مجلدات التطبيق"),QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NASAQ-"+QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")+".nasaqbackup","NASAQ backup (*.nasaqbackup)");};
 QObject::connect(exportBackup,&QPushButton::clicked,&d,[&]{run([&]{auto path=chooseBackup();if(path.isEmpty())return;backup(profilePath(),path);QSettings prefs("NASAQ","Preferences");prefs.setValue("last_verified_backup",path);prefs.setValue("last_verified_backup_time",QDateTime::currentDateTimeUtc());SilentMessageBox::information(&d,"NASAQ",text("Encrypted backup saved and verified.","تم حفظ النسخة المشفرة والتحقق من سلامتها."));});});
 auto restore=new QPushButton(text("Restore backup…","استعادة نسخة احتياطية…"));restore->setObjectName("restoreDataBackup");layout->addWidget(restore);
 QObject::connect(restore,&QPushButton::clicked,&d,[&]{run([&]{auto path=QFileDialog::getOpenFileName(&d,text("Select backup","اختيار نسخة احتياطية"),{},"NASAQ backup (*.nasaqbackup)");if(path.isEmpty())return;validateBackup(path);
  SilentMessageBox box(&d);box.setWindowTitle(text("Restore data","استعادة البيانات"));box.setText(text("Replace current data at the next start? First save a recovery backup. Playback will stop and NASAQ will close. External media files are unchanged.","استبدال البيانات عند التشغيل التالي؟ احفظ نسخة رجوع أولًا. سيتوقف التشغيل ويغلق نسق. ملفات الوسائط الخارجية لن تتغير."));box.setStandardButtons(QMessageBox::Yes|QMessageBox::No);box.setDefaultButton(QMessageBox::No);auto keep=new QCheckBox(text("Keep current license if present (uncheck to use backup license)","الاحتفاظ بالترخيص الحالي إن وجد (أزل العلامة لاستخدام ترخيص النسخة)"));keep->setChecked(true);box.setCheckBox(keep);if(box.exec()!=QMessageBox::Yes)return;
  auto recovery=chooseBackup();if(recovery.isEmpty())return;require(QFileInfo(recovery).absoluteFilePath().compare(QFileInfo(path).absoluteFilePath(),Qt::CaseInsensitive)!=0,"Recovery backup must use a different filename");backup(profilePath(),recovery);queueRestore(profilePath(),path,keep->isChecked());d.accept();QTimer::singleShot(0,qApp,&QCoreApplication::quit);
 });});
 auto reset=new QPushButton(text("Reset data…","إعادة ضبط البيانات…"));reset->setObjectName("resetNasaqData");layout->addWidget(reset);
 QObject::connect(reset,&QPushButton::clicked,&d,[&]{run([&]{SilentMessageBox box(&d);box.setWindowTitle(text("Reset NASAQ data","إعادة ضبط بيانات نسق"));box.setText(text("Delete NASAQ settings, playlists, schedules, imported assets and local logs at next start? First save a recovery backup. External music/video files and saved backup files are not deleted. NASAQ will close.","حذف إعدادات نسق وقوائمه وجداوله وملفاته المستوردة وسجلاته عند التشغيل التالي؟ احفظ نسخة رجوع أولًا. لا تُحذف الموسيقى والفيديو الخارجية ولا النسخ الاحتياطية المحفوظة. سيغلق نسق."));box.setStandardButtons(QMessageBox::Yes|QMessageBox::No);box.setDefaultButton(QMessageBox::No);auto keep=new QCheckBox(text("Keep the saved license on this device","الاحتفاظ بالترخيص المحفوظ على هذا الجهاز"));keep->setChecked(true);box.setCheckBox(keep);if(box.exec()!=QMessageBox::Yes)return;auto recovery=chooseBackup();if(recovery.isEmpty())return;backup(profilePath(),recovery);queueReset(profilePath(),keep->isChecked());d.accept();QTimer::singleShot(0,qApp,&QCoreApplication::quit);});});
 auto close=new QPushButton(text("Close","إغلاق"));layout->addWidget(close);QObject::connect(close,&QPushButton::clicked,&d,&QDialog::reject);d.exec();
}
}
