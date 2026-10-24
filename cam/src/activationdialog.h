// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>
#include "licenseactivation.h"
#include "nasaqlanguage.h"

class ActivationDialog:public QDialog {
 public:
 explicit ActivationDialog(Activation &activation,QWidget *parent=nullptr):QDialog(parent),activation(activation){
  setObjectName("activationDialog");setWindowTitle(text("License status","حالة الترخيص"));setMinimumWidth(500);setLayoutDirection(NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight);
  auto layout=new QVBoxLayout(this);layout->setContentsMargins(28,28,28,28);layout->setSpacing(18);
  title=new QLabel;title->setObjectName("brandHeading");layout->addWidget(title);
  message=new QLabel;message->setObjectName("activationMessage");message->setWordWrap(true);layout->addWidget(message);
  key=new QLineEdit;key->setObjectName("activationKey");key->setAccessibleName(text("License key","مفتاح الترخيص"));key->setLayoutDirection(Qt::LeftToRight);key->setPlaceholderText("XXXX-XXXX-XXXX-XXXX");key->setMaxLength(32);layout->addWidget(key);
  submit=new QPushButton(text("Activate","تفعيل"));submit->setObjectName("activateLicense");layout->addWidget(submit);
  remove=new QPushButton(text("Deactivate on this device","إلغاء التفعيل على هذا الجهاز"));remove->setObjectName("deactivateLicense");layout->addWidget(remove);
  connect(remove,&QPushButton::clicked,this,[this]{
   if(QMessageBox::question(this,text("Deactivate license","إلغاء التفعيل"),text("Remove the saved license from this device and enter another key? Playback will stop. Your playlists and schedules will be kept. This does not cancel the license or free a device slot on the licensing server.","هل تريد إزالة الترخيص المحفوظ من هذا الجهاز وإدخال مفتاح آخر؟ سيتوقف التشغيل مع الاحتفاظ بقوائم التشغيل والجداول. لا يلغي ذلك الترخيص نفسه ولا يحرر حصة الجهاز لدى خادم التراخيص."),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
   try{
    if(!this->activation.deactivateLocally()){QMessageBox::information(this,text("Please wait","يرجى الانتظار"),text("License verification is in progress. Try again when it finishes.","يجري التحقق من الترخيص. حاول مجددًا بعد اكتماله."));return;}
    key->clear();render(false);key->setFocus();
   }catch(const std::exception&){QMessageBox::warning(this,text("Could not deactivate","تعذر إلغاء التفعيل"),text("Could not update the saved license. The current license was kept.","تعذر تحديث الترخيص المحفوظ. تم الاحتفاظ بالترخيص الحالي."));}
  });
  auto close=new QPushButton(text("Close","إغلاق"));layout->addWidget(close);connect(close,&QPushButton::clicked,this,&QDialog::reject);
  connect(submit,&QPushButton::clicked,this,[this]{pending=true;submit->setEnabled(false);message->setText(text("Checking license…","جارٍ التحقق من الترخيص…"));this->activation.activate(key->text());});
  connect(&activation,&Activation::completed,this,[this](bool success){if(!pending)return;pending=false;if(success){key->clear();render(true);}else{submit->setEnabled(true);message->setText(NText(this->activation.description()));}});
  connect(&activation,&Activation::changed,this,[this]{if(!pending)render(false);});
  render(false);
 }
 private:
 QString text(const char *en,const char *ar)const{return QString::fromUtf8(NasaqLanguage()=="ar"?ar:en);}
 void render(bool success){
  const auto d=activation.details();key->setVisible(!d.valid);submit->setVisible(!d.valid);submit->setEnabled(!pending);remove->setVisible(d.valid);remove->setEnabled(!pending);
  title->setText(d.valid?text("License active","الترخيص مفعّل"):text("Activate NASAQ","تفعيل نسق"));
  if(!d.valid){message->setText(NText(activation.description())+"\n"+text("Enter your 16-character key. Internet is required for first activation.","أدخل مفتاحك المكوّن من 16 خانة. يلزم الإنترنت للتفعيل الأول."));return;}
  const auto days=qMax<qint64>(0,(QDateTime::currentDateTimeUtc().secsTo(d.expiresAt)+86399)/86400);
  const auto format=[](const QDateTime &v){return v.toLocalTime().toString("yyyy-MM-dd HH:mm t");};
  message->setText((success?text("Activation completed successfully.\n","تم التفعيل بنجاح.\n"):QString())+
   text("License expires: %1\nRemaining days (rounded up): %2\nDevice limit: %3\nOffline access until: %4","ينتهي الترخيص: %1\nالأيام المتبقية (مقربة للأعلى): %2\nعدد الأجهزة المسموح: %3\nالعمل دون إنترنت حتى: %4").arg(format(d.expiresAt)).arg(days).arg(d.maxMachines).arg(format(d.until)));
 }
 Activation &activation;QLabel *title,*message;QLineEdit *key;QPushButton *submit,*remove;bool pending=false;
};
