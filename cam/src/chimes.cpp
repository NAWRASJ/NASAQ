#include "silentmessagebox.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "formdesign.h"
#include "chimes.h"
#include "studio.h"
#include "nasaqlanguage.h"
#include "nasaqtheme.h"
#include <QtWidgets>
#include <QJsonArray>
static QString L(const char *en,const char *ar){return QString::fromUtf8(NasaqLanguage()=="ar"?ar:en);}
static QString root(){return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/chimes";}
QJsonArray Chimes::library(Store &s){
 auto items=QJsonDocument::fromJson(s.get("chime_library").toUtf8()).array();
 QFile manifest(QCoreApplication::applicationDirPath()+"/chimes/manifest.json");
 if(manifest.open(QIODevice::ReadOnly))for(auto value:QJsonDocument::fromJson(manifest.readAll()).array()){
  auto o=value.toObject();auto file=o["file"].toString();
  if(QFileInfo(file).fileName()!=file||file.isEmpty())continue;
  o["id"]="stock:"+file;o["file"]=QCoreApplication::applicationDirPath()+"/chimes/"+file;o["stock"]=true;items.append(o);
 }return items;
}
QString Chimes::resolve(Store &s,const QString &id){if(id.isEmpty()||id=="none")return {};for(auto v:library(s)){auto o=v.toObject();if(o["id"]==id)return o["file"].toString();}return {};}
int Chimes::gap(Store &s){return s.get("chime_gap").isEmpty()?300:qBound(0,s.get("chime_gap").toInt(),2000);}
QString Chimes::importFile(Store &s,const QString &file,const QString &name,const QString &source,const QString &libraryDirectory){
 if(QFileInfo(file).size()>20*1024*1024)throw std::runtime_error("Chime too large");
 auto duration=Studio::duration(file);if(duration>10000)throw std::runtime_error("Chime exceeds 10 seconds");
 Studio importer(s,libraryDirectory.isEmpty()?root():libraryDirectory);auto copy=importer.importAudio(file);auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
 auto items=QJsonDocument::fromJson(s.get("chime_library").toUtf8()).array();items.append(QJsonObject{{"id",id},{"file",copy},{"name",name.isEmpty()?QFileInfo(file).completeBaseName():name},{"source",source},{"duration",double(duration)}});
 s.set("chime_library",QString::fromUtf8(QJsonDocument(items).toJson(QJsonDocument::Compact)));return id;
}
ChimeCue::ChimeCue(QObject *p):QObject(p){player.setAudioOutput(&output);delay.setSingleShot(true);watchdog.setSingleShot(true);
 connect(&delay,&QTimer::timeout,this,[this]{auto done=std::move(continuation);continuation={};if(done)done();});
 connect(&watchdog,&QTimer::timeout,this,[this]{complete(true);});
 connect(&player,&QMediaPlayer::mediaStatusChanged,this,[this](auto st){if(st==QMediaPlayer::EndOfMedia)complete();});
 connect(&player,&QMediaPlayer::errorOccurred,this,[this]{complete(true);});
}
void ChimeCue::cancel(){continuation={};playingCue=false;delay.stop();watchdog.stop();player.stop();player.setSource({});}
void ChimeCue::start(const QString &file,float volume,int gap,std::function<void()> done){cancel();continuation=std::move(done);gapMs=qBound(0,gap,2000);
 if(file.isEmpty()){delay.start(0);return;}playingCue=true;
 if(!QFileInfo(file).isFile()){complete(true);return;}
 output.setVolume(qBound(0.f,volume,1.f));watchdog.start(15000);player.setSource(QUrl::fromLocalFile(file));player.play();
}
void ChimeCue::complete(bool failed){if(!playingCue||!continuation)return;playingCue=false;watchdog.stop();player.stop();if(failed&&error)error(player.errorString());delay.start(gapMs);}
void showChimes(QWidget *parent,Store &s){
 ApplyNasaqTheme();QDialog d(parent);d.setLayoutDirection(NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight);d.setWindowTitle(L("Announcement chimes","نغمات التنبيه"));d.resize(650,520);auto form=new QFormLayout(&d);
 auto info=new QLabel(L("Optional. Choose No chime to speak or announce directly. Imported files stay on this computer. Preview plays through the current output.","اختيارية. اختر بدون نغمة للتحدث أو بث الإعلان مباشرة. تُحفظ الملفات المستوردة على هذا الجهاز. المعاينة تُسمع عبر مخرج الصوت الحالي."));info->setWordWrap(true);form->addRow(info);
 auto list=new QListWidget;form->addRow(list);auto mic=new QComboBox,ads=new QComboBox;
 auto refill=[&]{list->clear();for(auto c:{mic,ads}){c->clear();c->addItem(L("No chime","بدون نغمة"),"none");}for(auto v:Chimes::library(s)){auto o=v.toObject();auto item=new QListWidgetItem(o["name"].toString(),list);item->setData(Qt::UserRole,o);item->setToolTip(o["source"].toString());for(auto c:{mic,ads})c->addItem(o["name"].toString(),o["id"].toString());}mic->setCurrentIndex(qMax(0,mic->findData(s.get("chime_mic"))));ads->setCurrentIndex(qMax(0,ads->findData(s.get("chime_ad"))));};refill();
 form->addRow(L("Microphone","الميكروفون"),mic);form->addRow(L("Announcements","الإعلانات"),ads);
 QSpinBox volume,gap;volume.setRange(0,100);volume.setSuffix(" %");volume.setValue(s.get("chime_volume").isEmpty()?70:s.get("chime_volume").toInt());gap.setRange(0,2000);gap.setSuffix(L(" ms"," مللي ثانية"));gap.setValue(Chimes::gap(s));form->addRow(L("Chime volume","مستوى النغمة"),&volume);form->addRow(L("Pause before speech","الفاصل قبل الكلام"),&gap);
 ChimeCue preview(&d);auto row=new QHBoxLayout;form->addRow(row);auto import=new QPushButton(L("Import","استيراد")),play=new QPushButton(L("Preview","معاينة")),remove=new QPushButton(L("Remove","إزالة"));for(auto b:{import,play,remove})row->addWidget(b);
 QObject::connect(play,&QPushButton::clicked,&d,[&]{if(auto item=list->currentItem())preview.start(item->data(Qt::UserRole).toJsonObject()["file"].toString(),volume.value()/100.f,0,[]{});});
 preview.error=[&](const QString&){info->setText(L("Cannot play this chime.","تعذر تشغيل النغمة."));};
 QObject::connect(import,&QPushButton::clicked,&d,[&]{auto file=QFileDialog::getOpenFileName(&d,L("Import chime (up to 10 seconds)","استيراد نغمة (حتى 10 ثوانٍ)"),{},"Audio (*.wav *.mp3 *.flac *.ogg *.m4a *.aac *.opus *.wma *.aiff)");if(file.isEmpty())return;bool ok=false;auto name=QInputDialog::getText(&d,L("Name","الاسم"),L("Chime name","اسم النغمة"),QLineEdit::Normal,QFileInfo(file).completeBaseName(),&ok);if(!ok)return;auto source=QInputDialog::getText(&d,L("Source","المصدر"),L("Source / license (optional for your own recording)","المصدر والترخيص (اختياري لتسجيلك الخاص)"));try{Chimes::importFile(s,file,name,source);refill();}catch(...){SilentMessageBox::warning(&d,L("Import failed","تعذر الاستيراد"),L("Use a decodable audio file, at most 10 seconds and 20 MB.","اختر ملفًا صوتيًا صالحًا لا يتجاوز 10 ثوانٍ و20 ميجابايت."));}});
 QObject::connect(remove,&QPushButton::clicked,&d,[&]{auto item=list->currentItem();if(!item)return;auto o=item->data(Qt::UserRole).toJsonObject();if(o["stock"].toBool())return;preview.cancel();QJsonArray kept;for(auto v:QJsonDocument::fromJson(s.get("chime_library").toUtf8()).array())if(v.toObject()["id"]!=o["id"])kept.append(v);s.set("chime_library",QString::fromUtf8(QJsonDocument(kept).toJson()));refill();});
 QDialogButtonBox buttons(QDialogButtonBox::Save|QDialogButtonBox::Close);buttons.button(QDialogButtonBox::Save)->setText(L("Save","حفظ"));buttons.button(QDialogButtonBox::Close)->setText(L("Close","إغلاق"));form->addRow(&buttons);QObject::connect(&buttons,&QDialogButtonBox::accepted,&d,[&]{s.set("chime_mic",mic->currentData().toString());s.set("chime_ad",ads->currentData().toString());s.set("chime_volume",QString::number(volume.value()));s.set("chime_gap",QString::number(gap.value()));d.accept();});QObject::connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);ApplyNasaqFormDesign(&d);d.exec();preview.cancel();
}
