// SPDX-License-Identifier: GPL-3.0-or-later
#include "formdesign.h"
#include "cloudspeechdialog.h"
#include "cloudspeech.h"
#include "nasaqlanguage.h"
#include <QtWidgets>
static QString L(const char *en,const char *ar){return QString::fromUtf8(NasaqLanguage()=="ar"?ar:en);}
QString createCloudAnnouncement(QWidget *parent,const QString &initialText,const QString &initialLanguage){
 QDialog dialog(parent);dialog.setObjectName("cloudSpeechDialog");dialog.setWindowTitle(L("Google cloud voice","صوت Google السحابي"));dialog.resize(680,650);
 QVBoxLayout outer(&dialog);QScrollArea scroll;scroll.setWidgetResizable(true);auto content=new QWidget;QFormLayout form(content);scroll.setWidget(content);outer.addWidget(&scroll);
 QLabel info(L("Create an announcement once, then play and schedule the saved file offline. Generation sends your text to Google and requires an enabled API and billing account. This window does not enable billing.","أنشئ الإعلان مرة واحدة، ثم شغّل الملف المحفوظ وجدوله دون إنترنت. يُرسل النص إلى Google عند التوليد، ويلزم تفعيل الخدمة وحساب الفوترة. هذه النافذة لا تفعّل الفوترة."));info.setWordWrap(true);form.addRow(&info);
 QSettings settings;QLineEdit project(settings.value("cloudSpeech/project").toString());project.setObjectName("cloudProject");project.setPlaceholderText("text-to-speech-386922");form.addRow(L("Google project ID","معرّف مشروع Google"),&project);
 QLineEdit token;token.setObjectName("cloudToken");token.setEchoMode(QLineEdit::Password);token.setPlaceholderText(L("Temporary access token — kept only while this window is open","رمز وصول مؤقت — يبقى فقط أثناء فتح هذه النافذة"));form.addRow(L("Connection token","رمز الاتصال"),&token);
 QComboBox language;language.addItem(L("Arabic","العربية"),"ar-XA");language.addItem(L("English (US)","الإنجليزية (أمريكا)"),"en-US");language.addItem(L("English (UK)","الإنجليزية (بريطانيا)"),"en-GB");language.addItem(L("Hindi","الهندية"),"hi-IN");language.setCurrentIndex(NasaqLanguage()=="ar"?0:1);if(!initialLanguage.isEmpty()){language.setCurrentIndex(language.findData(initialLanguage));language.setEnabled(false);}form.addRow(L("Speech language","لغة النطق"),&language);
 QComboBox voice;voice.setObjectName("cloudVoice");voice.addItems({"Aoede","Charon","Kore","Puck","Zephyr"});form.addRow(L("Chirp 3 HD voice","صوت Chirp 3 HD"),&voice);
 QPlainTextEdit text;text.setObjectName("cloudText");text.setMinimumHeight(140);text.setPlainText(initialText);if(!initialLanguage.isEmpty())text.setReadOnly(true);text.setPlaceholderText(L("Enter your announcement","اكتب نص الإعلان"));form.addRow(L("Announcement text","نص الإعلان"),&text);
 QLabel count;form.addRow(&count);QLabel cap(L("Local limit: 100,000 submitted characters per project per month on this installation. This is not a Google billing cap; usage elsewhere is not included. No automatic retries. Saved audio is reused for identical text and voice.","الحد المحلي: 100 ألف حرف مُرسل لكل مشروع شهريًا على هذه النسخة. ليس سقفًا لفاتورة Google ولا يشمل الاستخدام خارجها. لا توجد إعادة محاولة تلقائية. يُعاد استخدام الصوت المحفوظ عند تطابق النص والصوت."));cap.setWordWrap(true);form.addRow(&cap);
 QCheckBox consent(L("Send this text to Google when a saved copy is unavailable","إرسال هذا النص إلى Google عند عدم توفر نسخة محفوظة"));form.addRow(&consent);
 QLabel status;status.setWordWrap(true);status.setObjectName("cloudStatus");form.addRow(&status);
 QPushButton generate(L("Generate / reuse saved audio","توليد / استخدام الصوت المحفوظ")),cancel(L("Cancel request","إلغاء الطلب")),use(L("Use in announcements","استخدامه في الإعلانات"));generate.setObjectName("cloudGenerate");use.setObjectName("cloudUse");cancel.setEnabled(false);use.setEnabled(false);form.addRow(&generate);form.addRow(&cancel);form.addRow(&use);
 CloudSpeech service(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/announcements/google");QString selected;
 auto changed=[&]{selected.clear();use.setEnabled(false);count.setText(L("Text size (maximum 5,000 bytes): ","حجم النص (الحد 5000 بايت): ")+QString::number(text.toPlainText().toUtf8().size()));};
 QObject::connect(&text,&QPlainTextEdit::textChanged,&dialog,changed);QObject::connect(&language,&QComboBox::currentIndexChanged,&dialog,changed);QObject::connect(&voice,&QComboBox::currentIndexChanged,&dialog,changed);changed();
 auto busy=[&](bool value){for(QWidget *w:QList<QWidget*>{&generate,&text,&language,&voice,&project,&token,&consent})w->setEnabled(!value);cancel.setEnabled(value);if(!initialLanguage.isEmpty())language.setEnabled(false);};
 QObject::connect(&generate,&QPushButton::clicked,&dialog,[&]{
  // Consent is required only for a network request; the service checks cache
  // before credentials, so an empty token permits offline cache reuse.
  busy(true);use.setEnabled(false);status.setText(L("Preparing audio…","جارٍ تجهيز الصوت…"));settings.setValue("cloudSpeech/project",project.text().trimmed());
  service.generate(text.toPlainText(),language.currentData().toString(),language.currentData().toString()+"-Chirp3-HD-"+voice.currentText(),project.text().trimmed(),consent.isChecked()?token.text().trimmed():QString());
 });
 QObject::connect(&service,&CloudSpeech::ready,&dialog,[&](const QString &path,bool cached){busy(false);selected=path;use.setEnabled(true);status.setText(cached?L("Saved audio is ready. No cloud request was sent.","الصوت المحفوظ جاهز. لم يُرسل طلب سحابي."):L("Audio saved. Use it to preview or schedule the announcement.","حُفظ الصوت. استخدمه لمعاينة الإعلان أو جدولته."));});
 QObject::connect(&service,&CloudSpeech::failed,&dialog,[&](const QString &code){busy(false);selected.clear();use.setEnabled(false);
  const QMap<QString,QString> messages{
   {"TextLimit",L("Enter text within 5,000 UTF-8 bytes.","أدخل نصًا لا يتجاوز 5000 بايت بترميز UTF-8.")},
   {"VoiceInvalid",L("Choose a supported language and voice.","اختر لغة وصوتًا مدعومين.")},
   {"ProjectInvalid",L("Enter the Google project ID.","أدخل معرّف مشروع Google.")},
   {"TokenRequired",L("No saved audio found. Allow sending the text and enter a temporary access token.","لا يوجد صوت محفوظ. اسمح بإرسال النص وأدخل رمز وصول مؤقتًا.")},
   {"Authentication",L("The connection token is invalid or expired.","رمز الاتصال غير صالح أو انتهت صلاحيته.")},
   {"Permission",L("Check API enablement, billing and account permissions in Google Cloud.","تحقق من تفعيل الخدمة والفوترة وصلاحيات الحساب في Google Cloud.")},
   {"Quota",L("Google rejected the request because of a quota or rate limit.","رفض Google الطلب بسبب حد الحصة أو معدل الطلبات.")},
   {"LocalLimit",L("The local monthly limit has been reached.","تم بلوغ الحد الشهري المحلي.")},
   {"Cancelled",L("Request cancelled. Google may have counted a request already received.","أُلغي الطلب. قد يحتسب Google الطلب إذا كان قد استلمه.")},
   {"SaveFailed",L("Could not save audio or usage settings. Check disk access.","تعذر حفظ الصوت أو سجل الاستهلاك. تحقق من صلاحية الكتابة.")},
   {"InvalidAudio",L("An invalid audio response was rejected. No file was saved.","رُفض رد صوتي غير صالح. لم يُحفظ ملف.")}};
  status.setText(messages.value(code,L("Connection failed or timed out. No automatic retry was made.","فشل الاتصال أو انتهت مهلته. لم تُجرَ إعادة محاولة تلقائية.")));
 });
 QObject::connect(&cancel,&QPushButton::clicked,&service,&CloudSpeech::cancel);QObject::connect(&use,&QPushButton::clicked,&dialog,&QDialog::accept);
 QDialogButtonBox close(QDialogButtonBox::Close);outer.addWidget(&close);QObject::connect(&close,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
 ApplyNasaqFormDesign(&dialog);auto result=dialog.exec();service.cancel();token.clear();return result==QDialog::Accepted?selected:QString();
}
