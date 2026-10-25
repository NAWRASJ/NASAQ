#include "silentmessagebox.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "formdesign.h"
#include "livelanguage.h"
#include "window.h"
#include "packageidentity.h"
#include "windowsstartup.h"
#include "storeupdates.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include "nasaqtheme.h"
#include "nasaqlanguage.h"
#include "licenseactivation.h"
#include "activationdialog.h"
#include <QtWidgets>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPdfWriter>
#include <QDesktopServices>
#include <QVersionNumber>
static QByteArray pinHash(const QString&p,const QByteArray&s){return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,p.toUtf8(),s,210000,32);}
Window::Window(Store&s,MusicTransport*transport,QWidget*notificationHost):store(s),prayers(s,this),scheduler(s),audio(s,nullptr,transport),studio(s,QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/announcements/library"),studioEngine(studio,audio,this),activation(s,this){
 ApplyNasaqTheme();setWindowIcon(QIcon(":/nasaq/nasaq-128.png"));
 setWindowTitle(NText(QStringLiteral("Audio control")));resize(1080,720);
 setLayoutDirection(NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight);
 auto root=new QWidget;auto layout=new QVBoxLayout(root);layout->setContentsMargins(28,24,28,20);layout->setSpacing(16);auto scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidget(root);setCentralWidget(scroll);
 auto logo=new QLabel;logo->setPixmap(QPixmap(":/nasaq/nasaq-128.png").scaled(56,56,Qt::KeepAspectRatio,Qt::SmoothTransformation));auto brandRow=new QHBoxLayout;layout->addLayout(brandRow);
 auto heading=new QLabel(NText(QStringLiteral("NASAQ / نسق")));heading->setObjectName("brandHeading");brandRow->addWidget(logo);brandRow->addSpacing(12);brandRow->addWidget(heading,1);
 auto language=new QComboBox;language->addItem(NText(QStringLiteral("Follow Windows")),QStringLiteral("system"));language->addItem(QStringLiteral("العربية"),QStringLiteral("ar"));language->addItem(QStringLiteral("English"),QStringLiteral("en"));language->setObjectName("interfaceLanguage");language->setAccessibleName(NText(QStringLiteral("Language")));language->setCurrentIndex(qMax(0,language->findData(QSettings(QStringLiteral("NASAQ"),QStringLiteral("Preferences")).value(QStringLiteral("language"),QStringLiteral("system")))));connect(language,&QComboBox::activated,this,[this,language]{ApplyNasaqLanguage(language->currentData().toString());setLayoutDirection(NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight);setWindowTitle(NText("NASAQ")+" — "+sectionNames().value(navigation->currentRow()));refresh();status->clear();Q_EMIT interfaceLanguageChanged();});
 auto body=new QHBoxLayout;layout->addLayout(body,1);
 navigation=new QListWidget;navigation->setObjectName("sectionNavigation");navigation->setFixedWidth(195);navigation->setSpacing(4);navigation->setAccessibleName(NasaqLanguage()=="ar"?QStringLiteral("أقسام نسق"):QStringLiteral("NASAQ sections"));body->addWidget(navigation);
 sections=new QStackedWidget;sections->setObjectName("workspaceSections");body->addWidget(sections,1);
 QVector<QVBoxLayout*> pages;QVector<QGridLayout*> grids;
 const QStringList descriptions=NasaqLanguage()=="ar"?QStringList{
 QStringLiteral("تحكم بالميكروفون والإذاعة عبر المايك. لإدارة الأغاني وقوائم التشغيل، استخدم المشغّل الأساسي."),
 QStringLiteral("أنشئ إعلانًا من نص أو ملف صوتي، واحفظ نسخ اللغات واضبط تكرار بثها من الاستوديو."),
 QStringLiteral("راجع مواعيد التشغيل القادمة وأضف موعدًا أو ألغِ موعدًا محددًا."),
 QStringLiteral("اختر المدينة ومصدر المواقيت واضبط التوقف المؤقت والأذان الاختياري."),
 QStringLiteral("احفظ مستندات حقوق الموسيقى وصدّر سجل التشغيل المحلي."),
 QStringLiteral("إعدادات اللغة، وحماية التشغيل، وترخيص نسق، وتحديثات التطبيق.")}:QStringList{
 "Control microphone paging and live audio. Manage songs and playlists in the main player.",
 "Create announcements from text or audio, save language versions and schedule repeats in the studio.",
 "Review upcoming playback events, add an event or cancel a selected event.",
 "Choose your city and timetable source, and configure prayer pauses and optional adhan.",
 "Keep music-rights documents and export local playback records.",
 "Manage language, operator protection, NASAQ activation and application updates."};
 for(int i=0;i<6;++i){
  auto item=new QListWidgetItem(sectionNames()[i],navigation);item->setSizeHint(QSize(180,52));
  auto pane=new QWidget;pane->setObjectName(QString("section%1").arg(i));auto column=new QVBoxLayout(pane);column->setContentsMargins(20,8,8,8);column->setSpacing(18);
  auto title=new QLabel(sectionNames()[i]);title->setObjectName("sectionHeading");title->setStyleSheet("font-size:24px;font-weight:600;");column->addWidget(title);
  auto description=new QLabel(descriptions[i]);description->setWordWrap(true);description->setObjectName("subtitle");column->addWidget(description);
  auto grid=new QGridLayout;grid->setSpacing(12);column->addLayout(grid);pages<<column;grids<<grid;sections->addWidget(pane);
 }
 auto bar=grids[2];int toolIndex=0;
 auto button=[&](const QString&t,const std::function<void()>&fn){auto b=new QPushButton(t);bar->addWidget(b,toolIndex/3,toolIndex%3);++toolIndex;connect(b,&QPushButton::clicked,this,[this,fn]{guarded(fn);});};
 button(NText(QStringLiteral("جدولة / Schedule")),[this]{addEvent();});bar=grids[3];toolIndex=0;button(NText(QStringLiteral("أوقات الصلاة / Prayer times")),[this]{prayerSettings();});bar=grids[4];toolIndex=0;button(NText(QStringLiteral("سجل الحقوق / Music rights")),[this]{license();});
 button(NText(QStringLiteral("تصدير CSV")),[this]{auto f=QFileDialog::getSaveFileName(this,NText(QStringLiteral("Export logs")),QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NASAQ.csv","CSV (*.csv)");if(!f.isEmpty())store.exportCsv(f);});button(NText(QStringLiteral("تقرير PDF")),[this]{pdf();});
 auto pauseToggle=new QCheckBox(NText("Automatically pause music at adhan"));pauseToggle->setObjectName("adhanAutoPause");pauseToggle->setChecked(store.get("prayer_pause_enabled")!="0");pages[3]->addWidget(pauseToggle);
 auto timingNote=new QLabel(NText("Adhan is the start of the prayer time. Iqamah is mosque-specific and is not used here."));timingNote->setWordWrap(true);pages[3]->addWidget(timingNote);
 auto readiness=new QLabel;readiness->setObjectName("adhanReadiness");readiness->setWordWrap(true);pages[3]->addWidget(readiness);
 auto updateAdhan=[this,pauseToggle,readiness]{
  QSignalBlocker block(pauseToggle);const bool on=store.get("prayer_pause_enabled")!="0";pauseToggle->setChecked(on);const bool ar=NasaqLanguage()=="ar";
  const auto next=PrayerService::nextPause(store,QDateTime::currentDateTimeUtc());
  readiness->setText(!on?(ar?QStringLiteral("التوقف التلقائي: متوقف"):QStringLiteral("Automatic pause: off")):next.isValid()?(ar?QStringLiteral("التوقف التلقائي: مفعّل • الأذان القادم (%1): %2"):QStringLiteral("Automatic pause: on • Next adhan (%1): %2")).arg(store.get("prayer_city"),next.toLocalTime().toString("yyyy-MM-dd HH:mm t")):(ar?QStringLiteral("التوقف مفعّل، لكن لا توجد مواقيت محفوظة قادمة. افتح أوقات الأذان واختر المدينة ثم اضغط حفظ وتحديث المواقيت."):QStringLiteral("Pause is enabled, but no upcoming times are saved. Open Adhan times, choose the city, then Save and update prayer times.")));
 };
 connect(pauseToggle,&QCheckBox::clicked,this,[this,updateAdhan](bool on){guarded([this,on]{audio.setPrayerPauseEnabled(on);});updateAdhan();});
 auto adhanTimer=new QTimer(this);connect(adhanTimer,&QTimer::timeout,this,updateAdhan);adhanTimer->start(1000);connect(this,&Window::interfaceLanguageChanged,this,updateAdhan);updateAdhan();
 auto second=grids[0];int liveIndex=3;
 auto add=[&](QString t,auto fn){auto b=new QPushButton(t);second->addWidget(b,liveIndex/3,liveIndex%3);++liveIndex;if(t==NText(QStringLiteral("STOP ALL")))b->setObjectName("danger");if(t.contains(NText(QStringLiteral("Activate"))))b->setObjectName("primary");connect(b,&QPushButton::clicked,this,fn);};
 add(NText(QStringLiteral("STOP ALL")),[this]{try{audio.stop();}catch(const std::exception&e){status->setText(NText(QString::fromUtf8(e.what())));}});
 auto page=new QPushButton(NText(QStringLiteral("Hold to page / اضغط للتحدث")));page->setObjectName("holdToTalk");page->setAutoRepeat(false);page->setMinimumSize(360,136);page->setMaximumWidth(620);page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
 page->setCursor(Qt::PointingHandCursor);page->setProperty("pagingState","idle");
 page->setAccessibleDescription(NasaqLanguage()=="ar"?QStringLiteral("اضغط باستمرار للتحدث، ثم أفلت الزر لإنهاء الإذاعة عبر المايك."):QStringLiteral("Hold to speak, then release to end microphone paging."));
 second->addWidget(page,0,0,1,3);page->setEnabled(false);
 auto hint=new QLabel(NasaqLanguage()=="ar"?QStringLiteral("جهّز الميكروفون أولًا، ثم استمر بالضغط طوال التحدث. أفلت الزر عند الانتهاء."):QStringLiteral("Arm the microphone first, then hold the button while speaking. Release when finished."));hint->setWordWrap(true);hint->setObjectName("subtitle");second->addWidget(hint,2,0,1,3);
 add(NText(QStringLiteral("Arm microphone")),[this,page]{guarded([this,page]{if(SilentMessageBox::question(this,NText(QStringLiteral("Microphone")),NText(QStringLiteral("Use headphones or an isolated PA output to prevent feedback. Arm hold-to-talk?")))==SilentMessageBox::Yes)page->setEnabled(true);});});
 connect(page,&QPushButton::pressed,this,[this,page]{guarded([this,page]{if(page->isDown()&&!audio.page(true))throw std::runtime_error("Microphone unavailable. Check Windows Settings > Privacy & security > Microphone and the input/output devices.");});});
 connect(page,&QPushButton::released,this,[this]{try{audio.page(false);}catch(const std::exception&e){status->setText(NText(QString::fromUtf8(e.what())));}});
 second=grids[1];liveIndex=0;
 add(NText(QStringLiteral("TTS voice")),[this]{guarded([this]{auto voices=audio.voices();QStringList names;for(const auto&v:voices)names<<v.name()+" / "+v.locale().name();bool ok=false;auto chosen=QInputDialog::getItem(this,NText(QStringLiteral("Installed voices")),NText(QStringLiteral("Arabic/English require an installed matching voice")),names,0,false,&ok);if(ok&&names.contains(chosen))audio.setVoice(voices[names.indexOf(chosen)]);});});
 second=grids[5];liveIndex=0;
 add(NText(QStringLiteral("Set PIN")),[this]{guarded([this]{bool ok;auto p=QInputDialog::getText(this,NText(QStringLiteral("Set operator PIN")),NText(QStringLiteral("6–32 characters (local operator lock only)")),QLineEdit::Password,{},&ok);if(!ok)return;if(p.size()<6||p.size()>32)throw std::runtime_error("PIN length must be 6–32");QByteArray salt;for(int i=0;i<4;i++){quint32 n=QRandomGenerator::system()->generate();salt.append(reinterpret_cast<const char*>(&n),4);}store.set("pin_salt",salt.toHex());store.set("pin_hash",pinHash(p,salt).toHex());});});
 add(NText(QStringLiteral("Lock")),[this,page]{if(store.get("pin_hash").isEmpty()){status->setText(NText(QStringLiteral("Set a PIN first")));return;}locked=true;Q_EMIT operatorLockChanged(true);page->setEnabled(false);audio.page(false);status->setText(NText(QStringLiteral("Operator controls locked. STOP remains available.")));});
#ifdef Q_OS_WIN
 add(NasaqLanguage()=="ar"?QStringLiteral("التشغيل مع ويندوز"):QStringLiteral("Windows startup"),[this]{guarded([this]{
  bool accepted=false;
  const QStringList choices=NasaqLanguage()=="ar"?QStringList{QStringLiteral("تفعيل"),QStringLiteral("إيقاف")}:QStringList{QStringLiteral("Enable"),QStringLiteral("Disable")};
  auto choice=QInputDialog::getItem(this,NasaqLanguage()=="ar"?QStringLiteral("التشغيل مع ويندوز"):QStringLiteral("Windows startup"),NasaqLanguage()=="ar"?QStringLiteral("تشغيل نسق عند تسجيل الدخول"):QStringLiteral("Launch NASAQ at sign-in"),choices,0,false,&accepted);
  if(!accepted)return;
  auto watcher=new QFutureWatcher<QString>(this);
  connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher]{auto error=watcher->result();watcher->deleteLater();if(error.isEmpty())SilentMessageBox::information(this,"NASAQ",NasaqLanguage()=="ar"?QStringLiteral("تم تطبيق إعداد التشغيل مع ويندوز."):QStringLiteral("Windows startup setting applied."));else SilentMessageBox::warning(this,"NASAQ",(NasaqLanguage()=="ar"?QStringLiteral("تعذر تغيير التشغيل التلقائي. راجع إعدادات ويندوز > التطبيقات > بدء التشغيل.\n"):QStringLiteral("Could not change startup. Check Windows Settings > Apps > Startup.\n"))+error);});
  watcher->setFuture(QtConcurrent::run([enabled=choice==choices.first()]{return Nasaq::setWindowsStartup(enabled);}));
 });});
#endif
 auto defaultPlayer=new QPushButton;defaultPlayer->setObjectName("defaultAudioPlayer");pages[5]->addWidget(defaultPlayer);
 auto defaultHelp=new QLabel;defaultHelp->setWordWrap(true);pages[5]->addWidget(defaultHelp);
 auto defaultText=[defaultPlayer,defaultHelp]{const bool ar=NasaqLanguage()=="ar";defaultPlayer->setText(ar?QStringLiteral("اجعل نسق المشغّل الافتراضي"):QStringLiteral("Make NASAQ the default audio player"));defaultHelp->setText(ar?QStringLiteral("اختر نسق للملفات الصوتية في إعدادات ويندوز التي ستفتح. أثناء توقف الأذان تُضاف الملفات إلى القائمة دون تشغيلها."):QStringLiteral("Choose NASAQ for audio files in Windows Settings. Files opened during a prayer pause are added without starting playback."));};defaultText();connect(this,&Window::interfaceLanguageChanged,defaultPlayer,defaultText);
 connect(defaultPlayer,&QPushButton::clicked,this,[this]{if(!QDesktopServices::openUrl(QUrl(Nasaq::defaultAudioSettingsUri()))&&!QDesktopServices::openUrl(QUrl("ms-settings:defaultapps")))SilentMessageBox::warning(this,"NASAQ",NasaqLanguage()=="ar"?QStringLiteral("افتح إعدادات ويندوز > التطبيقات > التطبيقات الافتراضية، ثم اختر نسق."):QStringLiteral("Open Windows Settings > Apps > Default apps and choose NASAQ."));});
 auto storeUpdates=new StoreUpdates(Nasaq::storeManaged(),this);pages[5]->addWidget(storeUpdates);
 connect(this,&Window::interfaceLanguageChanged,storeUpdates,&StoreUpdates::updateLanguage);
 auto updateNotice=new QPushButton;updateNotice->setObjectName("storeUpdateNotice");layout->insertWidget(1,updateNotice);updateNotice->hide();
 auto updateNoticeText=[updateNotice]{updateNotice->setText(NasaqLanguage()=="ar"?QStringLiteral("يتوفر تحديث جديد — افتح Microsoft Store"):QStringLiteral("Update available — open Microsoft Store"));};updateNoticeText();
 connect(this,&Window::interfaceLanguageChanged,updateNotice,updateNoticeText);
 connect(storeUpdates,&StoreUpdates::availabilityChanged,updateNotice,&QWidget::setVisible);
 connect(updateNotice,&QPushButton::clicked,storeUpdates,[storeUpdates]{storeUpdates->findChild<QPushButton*>("openStoreUpdate")->click();});
 if(!Nasaq::storeManaged()) add(NText(QStringLiteral("Updates")),[this]{guarded([this]{updates();});});
 add(NasaqLanguage()=="ar"?QStringLiteral("حالة الترخيص"):QStringLiteral("License status"),[this]{guarded([this]{showActivationDialog();});});

 second=grids[1];liveIndex=1;
 add(NasaqLanguage()=="ar"?QStringLiteral("استوديو الإعلانات"):QStringLiteral("Announcement studio"),[this]{guarded([this]{announcements();});});
 add(NasaqLanguage()=="ar"?QStringLiteral("نغمات التنبيه"):QStringLiteral("Announcement chimes"),[this]{guarded([this]{showChimes(this,store);});});
 connect(&audio,&Audio::pagingState,this,[page](const QString &state){page->setProperty("pagingState",state);page->style()->unpolish(page);page->style()->polish(page);page->update();bool ar=NasaqLanguage()=="ar";page->setToolTip(state=="failed"?(ar?QStringLiteral("راجع إعدادات ويندوز > الخصوصية والأمان > الميكروفون، وتأكد من جهاز الإدخال."):QStringLiteral("Check Windows Settings > Privacy & security > Microphone and your input device.")):QString());page->setText(state=="waiting"?(ar?QStringLiteral("انتظر النغمة… استمر بالضغط"):QStringLiteral("Wait for chime… keep holding")):state=="live"?(ar?QStringLiteral("تحدث الآن — أفلت للإنهاء"):QStringLiteral("Speak now — release to finish")):state=="failed"?(ar?QStringLiteral("تعذر فتح الميكروفون"):QStringLiteral("Microphone unavailable")):NText(QStringLiteral("Hold to page / اضغط للتحدث")));});
 auto scheduleTitle=new QLabel(NText(QStringLiteral("الجدول القادم / Schedule")));pages[2]->addWidget(scheduleTitle);
 events=new QTableWidget;events->setAlternatingRowColors(true);events->setShowGrid(false);events->verticalHeader()->hide();events->setSelectionBehavior(QAbstractItemView::SelectRows);events->setMinimumHeight(160);events->setColumnCount(4);events->setHorizontalHeaderLabels({NText(QStringLiteral("Local time")),NText(QStringLiteral("Kind")),NText(QStringLiteral("Status")),NText(QStringLiteral("Payload"))});events->setEditTriggers(QAbstractItemView::NoEditTriggers);events->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);pages[2]->addWidget(events,1);
 auto cancelEvent=new QPushButton(NasaqLanguage()=="ar"?QStringLiteral("إلغاء الموعد المحدد"):QStringLiteral("Cancel selected event"));pages[2]->addWidget(cancelEvent);connect(cancelEvent,&QPushButton::clicked,this,[this]{guarded([this]{int row=events->currentRow();if(row<0)return;store.exec("UPDATE events SET status='cancelled' WHERE id=? AND status='pending'",{events->item(row,0)->data(Qt::UserRole)});refresh();});});
 status=new QLabel;status->setObjectName("status");status->setWordWrap(true);layout->addWidget(status);connect(&audio,&Audio::status,this,[this](const QString&m){status->setText(m.startsWith("adhan_auto_pause")?(NasaqLanguage()=="ar"?QStringLiteral("تم تحديث إعداد التوقف عند الأذان"):QStringLiteral("Adhan pause setting updated")):NText(m));});
 auto notice=new QLabel(NText(QStringLiteral("Compliance Lite stores operator records and local playback events. It does not certify licensing or regulatory compliance.")));notice->setObjectName("subtitle");notice->setWordWrap(true);pages[4]->addWidget(notice);
 auto licenceStatus=new QLabel(NText(activation.description()));licenceStatus->setObjectName("licenseStatus");licenceStatus->setWordWrap(true);pages[5]->addWidget(licenceStatus);
 connect(this,&Window::interfaceLanguageChanged,this,[this,licenceStatus]{licenceStatus->setText(NText(activation.description()));});
 auto languageRow=new QHBoxLayout;languageRow->addWidget(new QLabel(NText(QStringLiteral("Language"))));languageRow->addWidget(language,1);pages[5]->addLayout(languageRow);
 for(int i=0;i<pages.size();++i)if(i!=2)pages[i]->addStretch();
 connect(navigation,&QListWidget::currentRowChanged,this,[this,page](int index){
  audio.page(false);page->setDown(false);sections->setCurrentIndex(index);
  setWindowTitle(NText(QStringLiteral("NASAQ"))+" — "+sectionNames().value(index));
 });
 navigation->setCurrentRow(0);
 // During host construction Strawberry has not initialized its engine yet.
 audio.setLicenseAllowed(activation.allowed(),false);
 lastLicenseAllowed=activation.allowed();
 if(!lastLicenseAllowed && LicenseConfig::production().configured())QTimer::singleShot(800,this,[this]{if(!activation.allowed())showActivationDialog();});
 connect(&activation,&Activation::changed,this,[this,licenceStatus](bool allowed,const QString&message){const bool expired=lastLicenseAllowed&&!allowed;lastLicenseAllowed=allowed;audio.setLicenseAllowed(allowed);if(expired)QTimer::singleShot(0,this,[this]{showActivationDialog();});licenceStatus->setText(NText(message));Q_EMIT licenseAllowedChanged(allowed);});
 auto prayerPopup=new QDialog(notificationHost?notificationHost:this);prayerNotice=prayerPopup;prayerPopup->setObjectName("prayerPausePopup");prayerPopup->setWindowTitle(NasaqLanguage()=="ar"?QStringLiteral("توقف مؤقت بمناسبة الأذان"):QStringLiteral("Temporary pause for adhan"));prayerPopup->setWindowModality(Qt::WindowModal);prayerPopup->resize(480,240);
 auto prayerLayout=new QVBoxLayout(prayerPopup);auto prayerMessage=new QLabel(prayerPopup);prayerMessage->setWordWrap(true);prayerLayout->addWidget(prayerMessage);
 auto skipPrayer=new QPushButton(NasaqLanguage()=="ar"?QStringLiteral("تخطَّ وشغّل"):QStringLiteral("Skip and play"),prayerPopup);skipPrayer->setObjectName("skipPrayer");prayerLayout->addWidget(skipPrayer);
 auto keepPause=new QPushButton(NasaqLanguage()=="ar"?QStringLiteral("متابعة التوقف"):QStringLiteral("Keep paused"),prayerPopup);keepPause->setDefault(true);prayerLayout->addWidget(keepPause);
 connect(keepPause,&QPushButton::clicked,prayerPopup,&QDialog::hide);
 connect(skipPrayer,&QPushButton::clicked,this,[this,prayerPopup]{guarded([this,prayerPopup]{if(audio.skipPrayer())prayerPopup->hide();});});
 auto updatePrayerPopup=[this,prayerMessage,skipPrayer]{
  const bool ar=NasaqLanguage()=="ar";auto until=QDateTime::fromString(store.get("prayer_hold_until"),Qt::ISODate);auto seconds=qMax<qint64>(0,QDateTime::currentDateTimeUtc().secsTo(until));
  auto city=store.get("prayer_city");if(city.isEmpty())city=ar?QStringLiteral("المدينة المحددة"):QStringLiteral("the selected city");if(ar&&city=="Dubai")city=QStringLiteral("دبي");
  auto remaining=QString("%1:%2").arg(seconds/60,2,10,QChar('0')).arg(seconds%60,2,10,QChar('0'));
  prayerMessage->setText(ar?QStringLiteral("احترامًا لوقت الأذان بحسب مواقيت %1، أُوقفت الموسيقى مؤقتًا.\nسيُستأنف التشغيل السابق عند %2.\nالوقت المتبقي: %3\nشكرًا لتفهّمكم.").arg(city,until.toLocalTime().toString("HH:mm"),remaining):QStringLiteral("Music is temporarily paused for adhan according to the timetable for %1.\nPrevious playback will resume at %2.\nTime remaining: %3\nThank you for your understanding.").arg(city,until.toLocalTime().toString("HH:mm"),remaining));skipPrayer->setEnabled(activation.allowed());
 };
 auto prayerCountdown=new QTimer(prayerPopup);prayerCountdown->setInterval(1000);connect(prayerCountdown,&QTimer::timeout,prayerPopup,updatePrayerPopup);
 connect(&audio,&Audio::prayerPauseChanged,prayerPopup,[prayerPopup,prayerCountdown,updatePrayerPopup](const QDateTime &until){if(!until.isValid()){prayerCountdown->stop();prayerPopup->hide();return;}updatePrayerPopup();prayerCountdown->start();auto host=prayerPopup->parentWidget();
 if(host){if(host->isMinimized()||!host->isVisible())host->showNormal();host->raise();}
 prayerPopup->adjustSize();
 if(host){
  auto bounds=host->screen()->availableGeometry();auto pos=host->frameGeometry().center()-QPoint(prayerPopup->width()/2,prayerPopup->height()/2);
  pos.setX(qBound(bounds.left(),pos.x(),qMax(bounds.left(),bounds.right()-prayerPopup->width()+1)));
  pos.setY(qBound(bounds.top(),pos.y(),qMax(bounds.top(),bounds.bottom()-prayerPopup->height()+1)));
  prayerPopup->move(pos);
 }
 prayerPopup->show();prayerPopup->raise();prayerPopup->activateWindow();},Qt::QueuedConnection);
 auto timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{try{activation.tick();audio.tick(QDateTime::currentDateTimeUtc());scheduler.tick(QDateTime::currentDateTimeUtc(),[this](const Event&e){return audio.dispatch(e);});studioEngine.tick(QDateTime::currentDateTimeUtc());refresh();}catch(const std::exception&e){audio.setLicenseAllowed(false);Q_EMIT licenseAllowedChanged(false);status->setText(NText(QStringLiteral("Storage/scheduler error: "))+e.what());}});timer->start(1000);
 locked=!store.get("pin_hash").isEmpty();refresh();
 connect(&prayers,&PrayerService::completed,this,[this](bool ok,const QString&m){if(!ok)status->setText((NasaqLanguage()=="ar"?QStringLiteral("تعذر تحديث مواقيت الصلاة: "):QStringLiteral("Prayer update failed: "))+m);refresh();});
 auto q=store.query("SELECT issuer,expires FROM licenses WHERE expires<=?",{QDate::currentDate().addDays(30).toString(Qt::ISODate)});QStringList reminders;while(q.next())reminders<<q.value(0).toString()+" — "+q.value(1).toString();
 status->setText(reminders.isEmpty()?NText(QStringLiteral("Ready. Recovery preserves schedules; audio does not auto-start after restart.")):NText(QStringLiteral("Licence renewal due or expired: "))+reminders.join("; "));
}
Window::~Window(){delete prayerNotice.data();}
QStringList Window::sectionNames(){return NasaqLanguage()=="ar"?QStringList{QStringLiteral("الإذاعة عبر المايك"),QStringLiteral("الإعلانات"),QStringLiteral("الجدولة"),QStringLiteral("أوقات الأذان"),QStringLiteral("السجلات والتقارير"),QStringLiteral("الإعدادات")}:QStringList{"Microphone paging","Announcements","Schedule","Adhan times","Records & reports","Settings"};}
void Window::openSection(int index){if(index<0||index>=sections->count())return;navigation->setCurrentRow(index);show();raise();activateWindow();}
bool Window::unlock(){if(!locked)return true;auto until=QDateTime::fromString(store.get("pin_until"),Qt::ISODate);if(until>QDateTime::currentDateTimeUtc()){SilentMessageBox::warning(this,NText(QStringLiteral("Locked")),NText(QStringLiteral("Wait before trying again.")));return false;}bool ok;auto p=QInputDialog::getText(this,NText(QStringLiteral("Unlock")),NText(QStringLiteral("Operator PIN")),QLineEdit::Password,{},&ok);if(!ok)return false;auto h=pinHash(p,QByteArray::fromHex(store.get("pin_salt").toLatin1())).toHex();if(h!=store.get("pin_hash").toLatin1()){int n=store.get("pin_failures").toInt()+1;store.set("pin_failures",QString::number(n));if(n>=5)store.set("pin_until",QDateTime::currentDateTimeUtc().addSecs(60).toString(Qt::ISODate));return false;}store.set("pin_failures","0");store.set("pin_until","");locked=false;Q_EMIT operatorLockChanged(false);return true;}
void Window::guarded(const std::function<void()>&fn){try{if(unlock())fn();}catch(const std::exception&e){SilentMessageBox::warning(this,NText(QStringLiteral("NASAQ")),NText(QString::fromUtf8(e.what())));}}
void Window::refresh(){auto q=store.query("SELECT due,kind,status,payload,id FROM events ORDER BY CASE WHEN status='pending' THEN 0 ELSE 1 END,due ASC LIMIT 200");events->setRowCount(0);while(q.next()){int r=events->rowCount();events->insertRow(r);events->setItem(r,0,new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(q.value(0).toLongLong()).toString("yyyy-MM-dd HH:mm:ss")));events->item(r,0)->setData(Qt::UserRole,q.value(4));for(int i=1;i<4;i++)events->setItem(r,i,new QTableWidgetItem(i==3?(q.value(1).toString()=="tts"&&QJsonDocument::fromJson(q.value(i).toString().toUtf8()).object().contains("nasaqTts")?QJsonDocument::fromJson(q.value(i).toString().toUtf8()).object()["text"].toString():q.value(i).toString()):NText(q.value(i).toString())));}}
void Window::addEvent(){QDialog d(this);d.setWindowTitle(NText(QStringLiteral("Schedule one occurrence")));QFormLayout form(&d);QComboBox kind;for(const auto& code:QStringList{"music","announcement","adhan","tts"})kind.addItem(NText(code),code);QDateTimeEdit due(QDateTime::currentDateTime().addSecs(60));due.setDisplayFormat("yyyy-MM-dd HH:mm:ss");QPlainTextEdit payload;form.addRow(NText(QStringLiteral("Kind")),&kind);form.addRow(NText(QStringLiteral("Local time")),&due);form.addRow(NText(QStringLiteral("File path(s), one per line; or TTS text")),&payload);QPushButton browse(NText(QStringLiteral("Choose audio")));form.addRow(&browse);connect(&browse,&QPushButton::clicked,&d,[&]{payload.setPlainText(QFileDialog::getOpenFileNames(&d,NText(QStringLiteral("Audio"))).join('\n'));});QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);ApplyNasaqFormDesign(&d);if(d.exec()!=QDialog::Accepted)return;if(due.dateTime()<=QDateTime::currentDateTime()||payload.toPlainText().trimmed().isEmpty())throw std::runtime_error("Choose a future time and payload");if(kind.currentData()!="tts"){auto files=payload.toPlainText().split('\n',Qt::SkipEmptyParts);if(kind.currentData()!="music"&&files.size()!=1)throw std::runtime_error("Choose one audio file for an announcement");for(const auto &f:files)if(!QFileInfo(f).isFile())throw std::runtime_error("Choose an existing audio file");}store.add({QUuid::createUuid().toString(),kind.currentData().toString(),payload.toPlainText(),due.dateTime().toUTC(),kind.currentData().toString()=="adhan"?60:kind.currentData().toString()=="tts"?40:kind.currentData().toString()=="music"?10:50});refresh();}
void Window::importPrayers(){auto f=QFileDialog::getOpenFileName(this,NText(QStringLiteral("Import verified prayer timetable (UTC ISO timestamps)")),{},"JSON (*.json)");if(f.isEmpty())return;QFile file(f);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot read timetable");QJsonParseError err;auto doc=QJsonDocument::fromJson(file.readAll(),&err);if(err.error!=QJsonParseError::NoError||!doc.isArray())throw std::runtime_error("Expected JSON array");QList<Event> batch;QSet<QString> ids;for(auto item:doc.array()){auto o=item.toObject();auto id=o["id"].toString(),utc=o["utc"].toString(),path=o["audio"].toString();auto time=QDateTime::fromString(utc,Qt::ISODate);if(id.isEmpty()||ids.contains(id)||!utc.endsWith('Z')||!time.isValid()||time<=QDateTime::currentDateTimeUtc()||(!path.isEmpty()&&!QFileInfo(path).isFile()))throw std::runtime_error("Invalid/duplicate prayer ID, future UTC time, or missing adhan file");ids.insert(id);batch.append({"prayer:"+id,path.isEmpty()?"prayer":"adhan",path,time,60});}store.exec("BEGIN IMMEDIATE");try{for(auto&e:batch)store.add(e);store.exec("COMMIT");}catch(...){store.exec("ROLLBACK");throw;}refresh();}
void Window::license(){QDialog d(this);QFormLayout form(&d);QLineEdit issuer,ref;QDateEdit expiry(QDate::currentDate().addYears(1));expiry.setCalendarPopup(true);form.addRow(NText(QStringLiteral("Issuer")),&issuer);form.addRow(NText(QStringLiteral("Reference")),&ref);form.addRow(NText(QStringLiteral("Expiry")),&expiry);QDialogButtonBox b(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&b);connect(&b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&b,&QDialogButtonBox::rejected,&d,&QDialog::reject);ApplyNasaqFormDesign(&d);if(d.exec()!=QDialog::Accepted)return;if(issuer.text().trimmed().isEmpty()||ref.text().trimmed().isEmpty())throw std::runtime_error("Issuer and reference are required");auto source=QFileDialog::getOpenFileName(this,NText(QStringLiteral("Attach licence document (optional)")));QString dest;auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);if(!source.isEmpty()){auto dir=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/documents";QDir().mkpath(dir);dest=dir+"/"+id+"."+QFileInfo(source).suffix();if(!QFile::copy(source,dest))throw std::runtime_error("Document copy failed");}store.exec("INSERT INTO licenses VALUES(?,?,?,?,?)",{id,issuer.text(),ref.text(),expiry.date().toString(Qt::ISODate),dest});store.log("licence_record",id);status->setText(NText(QStringLiteral("Licence record saved; renewal reminders appear at startup.")));}
void Window::pdf(){auto path=QFileDialog::getSaveFileName(this,NText(QStringLiteral("Playback report")),QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NASAQ.pdf","PDF (*.pdf)");if(path.isEmpty())return;QString html=(NasaqLanguage()=="ar"?QString("<html dir='rtl'><h1>نسق — تقرير التشغيل المحلي</h1><p>سجلات المشغّل وليست شهادة امتثال. يمكن لحساب Windows المحلي تعديل السجلات.</p><table border='1'><tr><th>التوقيت العالمي</th><th>الحدث</th><th>التفاصيل</th></tr>"):QString("<h1>NASAQ — Local playback report</h1><p>Operator records; not regulatory certification. Logs are editable by the local Windows account.</p><table border='1'><tr><th>UTC</th><th>Event</th><th>Detail</th></tr>"));auto q=store.query("SELECT utc,kind,detail FROM logs ORDER BY id");while(q.next()){html+="<tr>";for(int i=0;i<3;i++)html+="<td>"+q.value(i).toString().toHtmlEscaped()+"</td>";html+="</tr>";}html+="</table>";QPdfWriter writer(path);writer.setPageSize(QPageSize::A4);QTextDocument doc;doc.setHtml(html);doc.print(&writer);if(!QFileInfo(path).size())throw std::runtime_error("PDF output failed");}
void Window::updates(){if(Nasaq::storeManaged())return;bool ok;auto repo=QInputDialog::getText(this,"GitHub Releases",NText(QStringLiteral("Release repository: owner/repo")),QLineEdit::Normal,store.get("release_repo"),&ok);if(!ok)return;if(!QRegularExpression("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$").match(repo).hasMatch())throw std::runtime_error("Invalid repository");store.set("release_repo",repo);auto manager=new QNetworkAccessManager(this);QNetworkRequest req(QUrl("https://api.github.com/repos/"+repo+"/releases/latest"));req.setRawHeader("Accept","application/vnd.github+json");req.setRawHeader("User-Agent","NASAQ-" NASAQ_VERSION_STRING);req.setTransferTimeout(10000);auto reply=manager->get(req);connect(reply,&QNetworkReply::finished,this,[this,reply,manager,repo]{if(reply->error()!=QNetworkReply::NoError){SilentMessageBox::warning(this,NText(QStringLiteral("Updates")),reply->errorString());}else{auto o=QJsonDocument::fromJson(reply->readAll()).object();auto tag=o["tag_name"].toString();auto version=QVersionNumber::fromString(tag.startsWith('v')?tag.mid(1):tag);if(version.isNull())SilentMessageBox::warning(this,NText(QStringLiteral("Updates")),NText(QStringLiteral("Release does not contain a semantic version tag")));else if(version>QVersionNumber::fromString(QCoreApplication::applicationVersion())){if(SilentMessageBox::question(this,NText(QStringLiteral("Update available")),tag+NText(QStringLiteral(" is available. Open the GitHub release page?")))==SilentMessageBox::Yes)QDesktopServices::openUrl(QUrl("https://github.com/"+repo+"/releases/latest"));}else SilentMessageBox::information(this,NText(QStringLiteral("Updates")),NText(QStringLiteral("No newer stable version found.")));}reply->deleteLater();manager->deleteLater();});}

bool Window::event(QEvent *event){if(event->type()==QEvent::WindowDeactivate||event->type()==QEvent::Hide)audio.page(false);return QMainWindow::event(event);}
void Window::closeEvent(QCloseEvent*event){audio.page(false);if(locked){event->ignore();status->setText(NText(QStringLiteral("Unlock operator controls before closing NASAQ.")));}else QMainWindow::closeEvent(event);}

void Window::showActivationDialog(){
if(activationDialogOpen)return;
 activationDialogOpen=true;
 ActivationDialog dialog(activation,this);
 const auto shot=qEnvironmentVariable("NASAQ_UI_SMOKE");if(!shot.isEmpty())QTimer::singleShot(300,&dialog,[&dialog,shot]{dialog.grab().save(shot+"-activation.png");dialog.reject();});dialog.exec();activationDialogOpen=false;
}
