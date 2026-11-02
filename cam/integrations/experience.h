// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>
#include <QMediaDevices>
#include "core/application.h"
#include "core/player.h"
#include "playlist/playlistmanager.h"
#include "window.h"
#include "audience.h"
#include "nasaqlanguage.h"
#include "nasaqtheme.h"

class NasaqSetupPage:public QWizardPage { public: using QWizardPage::QWizardPage; using QWizardPage::registerField; };

// Operator-only workspace. Audience surfaces continue to receive media/prayer frames only.
class NasaqExperience final : public QObject {
public:
 NasaqExperience(Player *player,PlaylistManager *manager,QMainWindow *main,Store &store,Window *window,AudienceDisplay *audience):QObject(main),player_(player),manager_(manager),main_(main),store_(store),window_(window),audience_(audience){
  auto bar=new QToolBar(main);bar->setObjectName(QStringLiteral("nasaqDailyWorkspace"));bar->setMovable(false);main->addToolBar(Qt::TopToolBarArea,bar);main->insertToolBarBreak(bar);
  auto panel=new QWidget(bar);panel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);auto layout=new QVBoxLayout(panel);layout->setContentsMargins(12,8,12,8);
  state_=new QLabel(panel);state_->setObjectName(QStringLiteral("playbackExplanation"));state_->setWordWrap(true);state_->setAccessibleName(L(QStringLiteral("Playback status"),QStringLiteral("حالة التشغيل")));auto header=new QWidget(panel);header->setLayoutDirection(Qt::LeftToRight);auto heading=new QHBoxLayout(header);heading->setContentsMargins(0,0,0,0);heading->setSpacing(16);state_->setLayoutDirection(main->layoutDirection());
  if(auto total=main->findChild<QLabel*>(QStringLiteral("playlistTotalDuration")))heading->addWidget(total,0,Qt::AlignLeft|Qt::AlignTop);
  heading->addWidget(state_,1);layout->addWidget(header);
  upcoming_=new QLabel(panel);upcoming_->setObjectName(QStringLiteral("dailyUpcoming"));upcoming_->setWordWrap(true);layout->addWidget(upcoming_);
  auto actions=new QHBoxLayout;layout->addLayout(actions);
  auto button=[&](const QString &text,const QString &name,auto fn){auto b=new QPushButton(text,panel);b->setObjectName(name);actions->addWidget(b);connect(b,&QPushButton::clicked,this,fn);return b;};
  play_=button(L(QStringLiteral("Play / pause"),QStringLiteral("تشغيل / إيقاف مؤقت")),QStringLiteral("dailyPlay"),[this]{player_->PlayPauseHelper();});
  button(L(QStringLiteral("Stop"),QStringLiteral("إيقاف")),QStringLiteral("dailyStop"),[this]{window_->stopAll();});
  retry_=button(L(QStringLiteral("Retry playback"),QStringLiteral("إعادة المحاولة")),QStringLiteral("dailyRetry"),[this]{if(canPlay())player_->Play();});
  restore_=button(L(QStringLiteral("Restore last session"),QStringLiteral("استعادة الجلسة السابقة")),QStringLiteral("restoreLastSession"),[this]{restoreSession();});
  button(L(QStringLiteral("Adhan"),QStringLiteral("الأذان")),QStringLiteral("dailyPrayer"),[this]{window_->configurePrayers();});
  button(L(QStringLiteral("Audience preview"),QStringLiteral("معاينة الجمهور")),QStringLiteral("dailyAudience"),[this]{audience_->showFloating();});
  bar->addWidget(panel);
  auto menu=main->menuBar()->addMenu(L(QStringLiteral("Workspace"),QStringLiteral("مساحة العمل")));menu->setObjectName(QStringLiteral("nasaqExperienceMenu"));
  menu->addAction(L(QStringLiteral("Getting started…"),QStringLiteral("الإعداد الأول…")),this,[this]{showWizard();});
  menu->addAction(L(QStringLiteral("Data, license and updates…"),QStringLiteral("البيانات والترخيص والتحديثات…")),this,[this]{window_->openDataCenter();});
  auto advanced=menu->addAction(L(QStringLiteral("Show advanced coverage details"),QStringLiteral("عرض تفاصيل التغطية المتقدمة")));advanced->setCheckable(true);advanced->setChecked(store_.get(QStringLiteral("ux_advanced"))==QStringLiteral("1"));
  if(auto old=main->findChild<QToolBar*>(QStringLiteral("businessDuration")))old->setVisible(advanced->isChecked());
  connect(advanced,&QAction::toggled,this,[this](bool on){store_.set(QStringLiteral("ux_advanced"),on?QStringLiteral("1"):QStringLiteral("0"));if(auto old=main_->findChild<QToolBar*>(QStringLiteral("businessDuration")))old->setVisible(on);});
  auto sizeMenu=menu->addMenu(L(QStringLiteral("Text size"),QStringLiteral("حجم النص")));auto group=new QActionGroup(sizeMenu);group->setExclusive(true);QSettings prefs(QStringLiteral("NASAQ"),QStringLiteral("Preferences"));
  for(int percent:{100,115,130}){auto action=sizeMenu->addAction(QString::number(percent)+QStringLiteral("%"));action->setCheckable(true);group->addAction(action);action->setChecked(prefs.value(QStringLiteral("text_scale"),100).toInt()==percent);connect(action,&QAction::triggered,this,[percent]{QSettings(QStringLiteral("NASAQ"),QStringLiteral("Preferences")).setValue(QStringLiteral("text_scale"),percent);ApplyNasaqTextScale();});}
  connect(window,&Window::audienceSettingsRequested,audience,&AudienceDisplay::settings);
  connect(player_,&Player::Error,this,[this](const QString &error){lastError_=error;update();});
  connect(player_,&Player::Playing,this,[this]{lastError_.clear();pendingRecovery_=false;update();});
  connect(player_,&Player::Stopped,this,[this]{if(!main_->property("nasaqExiting").toBool()&&!player_->GetCurrentItem()){try{store_.set(QStringLiteral("ux_recovery"),QStringLiteral(""));}catch(const std::exception &e){lastError_=QString::fromUtf8(e.what());}pendingRecovery_=false;update();}},Qt::QueuedConnection);
  pendingRecovery_=!store_.get(QStringLiteral("ux_recovery")).isEmpty();main_->installEventFilter(this);
  auto timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{try{update();if(++ticks_%10==0)checkpoint();}catch(const std::exception &e){state_->setText(L(QStringLiteral("Unable to save session: "),QStringLiteral("تعذر حفظ الجلسة: "))+QString::fromUtf8(e.what()));}});timer->start(1000);update();
  main_->findChild<QPushButton*>(QStringLiteral("dailyPrayer"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+P")));main_->findChild<QPushButton*>(QStringLiteral("dailyAudience"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+D")));
  connect(window_,&Window::interfaceLanguageChanged,this,[this,menu]{menu->setTitle(L(QStringLiteral("Workspace"),QStringLiteral("مساحة العمل")));play_->setText(L(QStringLiteral("Play / pause"),QStringLiteral("تشغيل / إيقاف مؤقت")));retry_->setText(L(QStringLiteral("Retry playback"),QStringLiteral("إعادة المحاولة")));restore_->setText(L(QStringLiteral("Restore last session"),QStringLiteral("استعادة الجلسة السابقة")));main_->findChild<QPushButton*>(QStringLiteral("dailyStop"))->setText(L(QStringLiteral("Stop"),QStringLiteral("إيقاف")));main_->findChild<QPushButton*>(QStringLiteral("dailyPrayer"))->setText(L(QStringLiteral("Adhan"),QStringLiteral("الأذان")));main_->findChild<QPushButton*>(QStringLiteral("dailyAudience"))->setText(L(QStringLiteral("Audience preview"),QStringLiteral("معاينة الجمهور")));update();});
  connect(window_,&Window::licenseAllowedChanged,this,[this](bool allowed){if(allowed&&store_.get(QStringLiteral("ux_setup_complete")).isEmpty()&&store_.get(QStringLiteral("prayer_city")).isEmpty())QTimer::singleShot(250,this,[this]{showWizard();});});
  QTimer::singleShot(1500,this,[this]{if(window_->licenseAllowed()&&qEnvironmentVariableIsEmpty("CAM_SMOKE_OUTPUT")&&store_.get(QStringLiteral("ux_setup_complete")).isEmpty()&&store_.get(QStringLiteral("prayer_city")).isEmpty())showWizard();});
 }
 static QString L(const QString &en,const QString &ar){return NasaqLanguage()==QStringLiteral("ar")?ar:en;}
 void showWizard(){
  if(wizard_) {wizard_->raise();return;}
  QWizard wizard(main_);wizard_=&wizard;wizard.setObjectName(QStringLiteral("nasaqFirstRun"));wizard.setWindowTitle(L(QStringLiteral("Set up NASAQ"),QStringLiteral("تجهيز نسق")));wizard.resize(700,520);wizard.setButtonText(QWizard::NextButton,L(QStringLiteral("Next"),QStringLiteral("التالي")));wizard.setButtonText(QWizard::BackButton,L(QStringLiteral("Back"),QStringLiteral("السابق")));wizard.setButtonText(QWizard::FinishButton,L(QStringLiteral("Finish"),QStringLiteral("إنهاء")));wizard.setButtonText(QWizard::CancelButton,L(QStringLiteral("Later"),QStringLiteral("لاحقًا")));
  auto page=[&](const QString &title,const QString &help){auto p=new NasaqSetupPage(&wizard);p->setTitle(title);auto l=new QVBoxLayout(p);auto text=new QLabel(help,p);text->setWordWrap(true);l->addWidget(text);wizard.addPage(p);return l;};
  auto language=page(L(QStringLiteral("1. Language"),QStringLiteral("1. اللغة")),L(QStringLiteral("Choose your interface language. You can change it again in Settings."),QStringLiteral("اختر لغة الواجهة. يمكنك تغييرها لاحقًا في الإعدادات.")));
  auto languages=new QComboBox;languages->addItem(QStringLiteral("العربية"),QStringLiteral("ar"));languages->addItem(QStringLiteral("English"),QStringLiteral("en"));languages->setCurrentIndex(NasaqLanguage()==QStringLiteral("ar")?0:1);language->addWidget(languages);
  auto prayer=page(L(QStringLiteral("2. City and adhan"),QStringLiteral("2. المدينة والأذان")),L(QStringLiteral("Set the city, verify the timetable, and choose the pause duration and optional adhan file."),QStringLiteral("حدد المدينة وتحقق من المواقيت، ثم اختر مدة التوقف وملف الأذان الاختياري.")));
  auto prayerButton=new QPushButton(L(QStringLiteral("Configure adhan"),QStringLiteral("إعداد الأذان")));prayer->addWidget(prayerButton);connect(prayerButton,&QPushButton::clicked,&wizard,[this]{window_->configurePrayers();});
  auto outputs=page(L(QStringLiteral("3. Audio and audience screen"),QStringLiteral("3. الصوت وشاشة الجمهور")),L(QStringLiteral("Choose the audio output in player settings. Use audience preview to check the screen; an external display is optional."),QStringLiteral("اختر مخرج الصوت في إعدادات المشغّل. استخدم المعاينة لفحص شاشة الجمهور؛ الشاشة الخارجية اختيارية.")));
  auto output=new QPushButton(L(QStringLiteral("Audio output settings"),QStringLiteral("إعدادات مخرج الصوت")));outputs->addWidget(output);connect(output,&QPushButton::clicked,&wizard,[this]{if(auto a=main_->findChild<QAction*>(QStringLiteral("action_settings")))a->trigger();});
  auto screens=new QPushButton(L(QStringLiteral("Audience screen and images"),QStringLiteral("شاشة الجمهور والصور")));outputs->addWidget(screens);connect(screens,&QPushButton::clicked,&wizard,[this]{audience_->settings();});
  auto source=page(L(QStringLiteral("4. Playback source"),QStringLiteral("4. مصدر التشغيل")),L(QStringLiteral("Add audio/video files to the current playlist, or open your existing radio collection in the main player. Nothing starts automatically."),QStringLiteral("أضف ملفات صوت أو فيديو إلى قائمة التشغيل الحالية، أو افتح محطات الراديو من المشغّل الرئيسي. لن يبدأ التشغيل تلقائيًا.")));
  auto files=new QPushButton(L(QStringLiteral("Add audio / video files"),QStringLiteral("إضافة ملفات صوت / فيديو")));source->addWidget(files);connect(files,&QPushButton::clicked,&wizard,[this,&wizard]{auto paths=QFileDialog::getOpenFileNames(&wizard,L(QStringLiteral("Choose media"),QStringLiteral("اختيار الوسائط")));if(paths.isEmpty())return;QList<QUrl> urls;for(const auto &p:paths)urls<<QUrl::fromLocalFile(p);manager_->InsertUrls(manager_->current_id(),urls,-1,false,false,true);});
  auto radio=new QPushButton(L(QStringLiteral("Add a radio URL"),QStringLiteral("إضافة رابط محطة راديو")));source->addWidget(radio);auto radioNote=new QLabel;radioNote->setWordWrap(true);source->addWidget(radioNote);connect(radio,&QPushButton::clicked,&wizard,[this,&wizard,radioNote]{bool ok=false;auto text=QInputDialog::getText(&wizard,L(QStringLiteral("Radio station"),QStringLiteral("محطة الراديو")),L(QStringLiteral("Direct HTTP/HTTPS stream URL"),QStringLiteral("رابط البث المباشر HTTP/HTTPS")),QLineEdit::Normal,{},&ok);if(!ok)return;QUrl url(text.trimmed());if(!url.isValid()||url.host().isEmpty()||(url.scheme()!=QStringLiteral("http")&&url.scheme()!=QStringLiteral("https"))){radioNote->setText(L(QStringLiteral("Enter a valid HTTP/HTTPS stream URL."),QStringLiteral("أدخل رابط بث صالحًا يبدأ بـ HTTP أو HTTPS.")));return;}manager_->InsertUrls(manager_->current_id(),{url},-1,false,false,true);radioNote->setText(L(QStringLiteral("Station added to the playlist."),QStringLiteral("أُضيفت المحطة إلى قائمة التشغيل.")));});
  auto verify=page(L(QStringLiteral("5. Check playback"),QStringLiteral("5. تجربة التشغيل")),L(QStringLiteral("Start a short playback test, confirm the sound comes from the correct speakers, then test the adhan pause. The adhan test lasts 15 seconds. If you stop manually, it will not restart playback."),QStringLiteral("ابدأ تجربة تشغيل قصيرة وتأكد من خروج الصوت من السماعات المطلوبة، ثم اختبر توقف الأذان لمدة 15 ثانية. الإيقاف اليدوي يمنع الاستئناف.")));
  auto play=new QPushButton(L(QStringLiteral("Play / pause"),QStringLiteral("تشغيل / إيقاف مؤقت")));verify->addWidget(play);connect(play,&QPushButton::clicked,&wizard,[this]{player_->PlayPauseHelper();});
  auto test=new QPushButton(L(QStringLiteral("Test adhan for 15 seconds"),QStringLiteral("تجربة الأذان لمدة 15 ثانية")));verify->addWidget(test);auto feedback=new QLabel;feedback->setWordWrap(true);verify->addWidget(feedback);connect(test,&QPushButton::clicked,&wizard,[this,feedback]{feedback->setText(window_->previewPrayer()?L(QStringLiteral("Test started. Watch the audience preview."),QStringLiteral("بدأت التجربة. راقب معاينة الجمهور.")):L(QStringLiteral("Test unavailable. Check license, audio file or active prayer/announcement."),QStringLiteral("التجربة غير متاحة. تحقق من الترخيص والملف أو وجود صلاة/إعلان جارٍ.")));audience_->showFloating();});
  auto confirmed=new QCheckBox(L(QStringLiteral("I checked the sound and display"),QStringLiteral("تحققت من الصوت والعرض")));verify->addWidget(confirmed);static_cast<NasaqSetupPage*>(verify->parentWidget())->registerField(QStringLiteral("verified*"),confirmed);
  if(wizard.exec()==QDialog::Accepted){window_->changeInterfaceLanguage(languages->currentData().toString());store_.set(QStringLiteral("ux_setup_complete"),QStringLiteral("1"));}wizard_=nullptr;
 }
protected:
 bool eventFilter(QObject *object,QEvent *event) override {if(object==main_&&event->type()==QEvent::DynamicPropertyChange&&static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName()==QByteArrayLiteral("nasaqExiting")){try{checkpoint();}catch(const std::exception &e){lastError_=QString::fromUtf8(e.what());}}return QObject::eventFilter(object,event);}
private:
 bool canPlay()const{return window_->licenseAllowed()&&!window_->operatorLocked()&&!window_->prayerActive()&&window_->foregroundPriority()==0&&!player_->property("camSuspended").toBool();}
 void checkpoint(){
  if(window_->prayerActive()||player_->property("camSuspended").toBool()||pendingRecovery_)return;
  const auto state=player_->GetState();if(state!=EngineBase::State::Playing&&state!=EngineBase::State::Paused)return;
  auto list=manager_->active();auto item=player_->GetCurrentItem();if(!list||!item)return;
  QJsonObject saved{{QStringLiteral("playlist"),list->id()},{QStringLiteral("url"),item->OriginalUrl().toString()},{QStringLiteral("position"),double(qMax<qint64>(0,player_->engine()->position_nanosec()/1000000000))},{QStringLiteral("title"),item->EffectiveMetadata().PrettyTitle()},{QStringLiteral("radio"),item->EffectiveMetadata().is_radio()}};
  store_.set(QStringLiteral("ux_recovery"),QString::fromUtf8(QJsonDocument(saved).toJson(QJsonDocument::Compact)));
 }
 void restoreSession(){
  if(!canPlay())return;auto saved=QJsonDocument::fromJson(store_.get(QStringLiteral("ux_recovery")).toUtf8()).object();auto list=manager_->playlist(saved[QStringLiteral("playlist")].toInt(-1));
  if(list)for(int row=0;row<list->rowCount();++row)if(list->item_at(row)->OriginalUrl().toString()==saved[QStringLiteral("url")].toString()){
   manager_->SetCurrentPlaylist(list->id());manager_->SetActivePlaylist(list->id());player_->PlayAt(row,false,saved[QStringLiteral("radio")].toBool()?0:quint64(qBound(0.,saved[QStringLiteral("position")].toDouble(),double(7*86400)))*1000000000,EngineBase::TrackChangeType::Manual,Playlist::AutoScroll::Always,false);return;
  }
  lastError_=L(QStringLiteral("The saved item is no longer in its playlist. Add it again or choose another source."),QStringLiteral("العنصر المحفوظ لم يعد موجودًا في قائمته. أضفه مجددًا أو اختر مصدرًا آخر."));pendingRecovery_=false;update();
 }
 void update(){
  const auto now=QDateTime::currentDateTimeUtc();auto player=player_;const auto item=player->GetCurrentItem();const auto state=player->GetState();QString status;
  if(!window_->licenseAllowed())status=L(QStringLiteral("Activation required — open Data, license and updates"),QStringLiteral("يلزم التفعيل — افتح البيانات والترخيص والتحديثات"));
  else if(window_->prayerActive()){
   const auto seconds=qMax<qint64>(0,now.secsTo(window_->prayerDeadline()));status=(window_->prayerPreviewActive()?L(QStringLiteral("Adhan test — "),QStringLiteral("تجربة الأذان — ")):L(QStringLiteral("Paused for adhan — "),QStringLiteral("متوقف للأذان — ")))+QString(QStringLiteral("%1:%2")).arg(seconds/60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
   status+=window_->resumePending()&&item?L(QStringLiteral(" • Previous playback will resume"),QStringLiteral(" • سيُستأنف التشغيل السابق")):L(QStringLiteral(" • No automatic resume"),QStringLiteral(" • لن يبدأ التشغيل تلقائيًا"));
  }else if(window_->foregroundPriority()>0)status=window_->foregroundPriority()>=100?L(QStringLiteral("Microphone paging is active"),QStringLiteral("الإذاعة عبر الميكروفون جارية")):L(QStringLiteral("Announcement is playing — previous playback is temporarily held"),QStringLiteral("الإعلان قيد التشغيل — المصدر السابق متوقف مؤقتًا"));
  else if(player->property("nasaqRadioReconnecting").toBool())status=L(QStringLiteral("Radio disconnected — retrying automatically. You can stop or choose another station."),QStringLiteral("انقطع الراديو — تجري إعادة الاتصال تلقائيًا. يمكنك الإيقاف أو اختيار محطة أخرى."));
  else if(player->engine()->property("nasaqBuffering").toBool())status=L(QStringLiteral("Buffering — waiting for media"),QStringLiteral("جارٍ التحميل المؤقت — بانتظار الوسائط"));
  else if(!lastError_.isEmpty())status=L(QStringLiteral("Unable to play this source. Retry or choose another file/station."),QStringLiteral("تعذر تشغيل هذا المصدر. أعد المحاولة أو اختر ملفًا/محطة أخرى."));
  else if(state==EngineBase::State::Playing)status=L(QStringLiteral("Now playing: "),QStringLiteral("يعمل الآن: "))+(item?item->EffectiveMetadata().PrettyTitle():QString());
  else if(state==EngineBase::State::Paused)status=L(QStringLiteral("Paused manually — press Play to continue"),QStringLiteral("متوقف مؤقتًا — اضغط تشغيل للاستكمال"));
  else status=pendingRecovery_?L(QStringLiteral("Last session is available — restore when ready"),QStringLiteral("الجلسة السابقة متاحة — استعدها عندما تكون جاهزًا")):L(QStringLiteral("Playback stopped — no automatic restart"),QStringLiteral("التشغيل متوقف — لن يبدأ تلقائيًا"));
  state_->setText(status);state_->setToolTip(lastError_);restore_->setVisible(pendingRecovery_);restore_->setEnabled(canPlay());retry_->setVisible(!lastError_.isEmpty()||player->property("nasaqRadioReconnecting").toBool());retry_->setEnabled(canPlay());play_->setEnabled(canPlay());
  const auto next=PrayerService::nextPause(store_,now);QString info=L(QStringLiteral("Next adhan pause: "),QStringLiteral("التوقف القادم للأذان: "))+(next.isValid()?next.toLocalTime().toString(QStringLiteral("HH:mm")):L(QStringLiteral("Set city and timetable"),QStringLiteral("حدد المدينة والمواقيت")));
  if(store_.get(QStringLiteral("prayer_pause_enabled"))==QStringLiteral("0"))info=L(QStringLiteral("Automatic adhan pause is off"),QStringLiteral("التوقف التلقائي للأذان غير مفعّل"));
  auto q=store_.query(QStringLiteral("SELECT due FROM (SELECT due FROM events WHERE kind IN ('announcement','tts','sequence') AND status='pending' UNION ALL SELECT next AS due FROM studio_rules WHERE state='active') WHERE due>=? ORDER BY due LIMIT 1"),{now.toMSecsSinceEpoch()});info+=L(QStringLiteral(" • Next announcement: "),QStringLiteral(" • الإعلان القادم: "))+(q.next()?QDateTime::fromMSecsSinceEpoch(q.value(0).toLongLong()).toLocalTime().toString(QStringLiteral("HH:mm")):L(QStringLiteral("None scheduled"),QStringLiteral("لا يوجد موعد")));auto list=manager_->current();if(list)info+=L(QStringLiteral(" • Playlist items: "),QStringLiteral(" • عناصر القائمة: "))+QString::number(list->rowCount());upcoming_->setText(info);
 }
 Player *player_;PlaylistManager *manager_;QMainWindow *main_;Store &store_;Window *window_;AudienceDisplay *audience_;QLabel *state_,*upcoming_;QPushButton *play_,*retry_,*restore_;QPointer<QWizard> wizard_;QString lastError_;bool pendingRecovery_=false;int ticks_=0;
};
