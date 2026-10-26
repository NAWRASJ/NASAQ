// SPDX-License-Identifier: GPL-3.0-or-later
#include "audience.h"
#include "nasaqlanguage.h"
#include "formdesign.h"
#include "turntable.h"
#include "../../src/utilities/nasaqmedia.h"
#include "prayers.h"
#include "../../src/utilities/nasaqicons.h"
#include <QtWidgets>
#include <QImageReader>
#include <QCryptographicHash>
#include <QSaveFile>
#include <stdexcept>
#include <cmath>
namespace {
QString trAudience(const char *en,const char *ar){return QString::fromUtf8(NasaqLanguage()=="ar"?ar:en);}
QIcon videoIcon(const QString &name){
 QPixmap pixmap(32,32);pixmap.fill(Qt::transparent);QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);
 p.setPen(QPen(QColor("#667047"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
 if(name=="pause"){p.drawRoundedRect(QRectF(10,8,3,16),1,1);p.drawRoundedRect(QRectF(19,8,3,16),1,1);}
 else if(name=="stop")p.drawRoundedRect(QRectF(9,9,14,14),2,2);
 else if(name=="full"){
  for(int x:{8,24})for(int y:{8,24}){p.drawLine(x,y,x+(x==8?6:-6),y);p.drawLine(x,y,x,y+(y==8?6:-6));}
 }else{
  if(name=="previous"){p.translate(32,0);p.scale(-1,1);}
  QPolygonF triangle;triangle<<QPointF(11,8)<<QPointF(23,16)<<QPointF(11,24);p.drawPolygon(triangle);
  if(name=="next"||name=="previous")p.drawLine(25,8,25,24);
 }
 return QIcon(pixmap);
}
QString videoTime(qint64 seconds){seconds=qMax<qint64>(0,seconds);return seconds>=3600?QString("%1:%2:%3").arg(seconds/3600).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0')):QString("%1:%2").arg(seconds/60).arg(seconds%60,2,10,QLatin1Char('0'));}
QImage loadImage(const QString &path){
 if(path.isEmpty())return {};
 QImageReader reader(path);const auto size=reader.size();
 const auto format=reader.format().toLower();if(format!="png"&&format!="jpeg"&&format!="webp")return {};
 if(!size.isValid()||size.width()>16000||size.height()>16000||QFileInfo(path).size()>20*1024*1024)return {};
 reader.setAutoTransform(true);reader.setScaledSize(size.scaled(1920,1080,Qt::KeepAspectRatio));return reader.read();
}
}
AudienceSurface::AudienceSurface(QWidget *parent):QWidget(parent){
 Q_INIT_RESOURCE(nasaq_brand);
 setObjectName("nasaqAudienceSurface");setFocusPolicy(Qt::NoFocus);setContextMenuPolicy(Qt::PreventContextMenu);
 setCursor(Qt::BlankCursor);setAttribute(Qt::WA_OpaquePaintEvent);setMinimumSize(160,90);
}
void AudienceSurface::present(const QImage &frame,bool prayer,bool blocked,bool crop,const QImage &waiting,const QImage &prayerImage){
 const bool wasVideo=controlsAvailable();
 frame_=frame;prayer_=prayer;blocked_=blocked;crop_=crop;waiting_=waiting;prayerImage_=prayerImage;update();
 syncControls(wasVideo);
}
void AudienceSurface::enableVideoControls(){
 if(controls_)return;
 setMouseTracking(true);installEventFilter(this);controls_=new QWidget(this);controls_->setObjectName("videoControls");controls_->setAttribute(Qt::WA_StyledBackground);
 controls_->setLayoutDirection(Qt::LeftToRight);
 controls_->setStyleSheet("QWidget#videoControls{background:rgba(249,247,234,220);border:1px solid rgba(255,255,245,180);border-radius:20px;} QToolButton{background:transparent;border:1px solid transparent;border-radius:14px;padding:3px;} QToolButton:hover{background:rgba(223,226,202,190);border-color:rgba(102,112,71,65);} QToolButton:pressed{background:rgba(200,207,175,210);} QLabel{color:#626b46;background:transparent;border:0;font-size:12px;} QSlider{background:transparent;} QSlider::groove:horizontal{height:3px;background:rgba(102,112,71,48);border-radius:1px;} QSlider::sub-page:horizontal{background:#78815b;border-radius:1px;} QSlider::handle:horizontal{width:12px;margin:-5px 0;background:#faf8ed;border:2px solid #78815b;border-radius:7px;}");
 opacity_=new QGraphicsOpacityEffect(controls_);opacity_->setOpacity(1);controls_->setGraphicsEffect(opacity_);
 fade_=new QPropertyAnimation(opacity_,"opacity",this);fade_->setDuration(350);fade_->setEasingCurve(QEasingCurve::InOutCubic);
 connect(fade_,&QPropertyAnimation::finished,this,[this]{if(opacity_->opacity()==0){controls_->hide();setCursor(Qt::BlankCursor);}});
 auto layout=new QVBoxLayout(controls_);layout->setContentsMargins(20,12,20,10);layout->setSpacing(4);
 seek_=new QSlider(Qt::Horizontal,controls_);seek_->setObjectName("videoSeek");seek_->setFocusPolicy(Qt::TabFocus);seek_->setRange(0,10000);seek_->setFixedHeight(22);seek_->setAccessibleName(trAudience("Playback position","موضع التشغيل"));layout->addWidget(seek_);
 auto row=new QHBoxLayout;row->setSpacing(5);layout->addLayout(row);
 auto button=[&](const char *name,const QString &label){auto b=new QToolButton(controls_);b->setObjectName(QString("video_%1").arg(name));b->setIcon(videoIcon(name));b->setIconSize(QSize(28,28));b->setFixedSize(38,38);b->setToolTip(label);b->setAccessibleName(label);b->setFocusPolicy(Qt::TabFocus);row->addWidget(b);return b;};
 auto previous=button("previous",trAudience("Previous","السابق"));playButton_=button("play",trAudience("Play / pause","تشغيل / إيقاف مؤقت"));auto next=button("next",trAudience("Next","التالي"));auto stop=button("stop",trAudience("Stop","إيقاف"));
 row->addStretch();time_=new QLabel(controls_);time_->setObjectName("videoTime");row->addWidget(time_);row->addSpacing(8);volume_=new QSlider(Qt::Horizontal,controls_);volume_->setObjectName("mediaVolume");volume_->setFocusPolicy(Qt::TabFocus);volume_->setRange(0,100);volume_->setFixedWidth(70);volume_->setAccessibleName(trAudience("Volume","مستوى الصوت"));volume_->setToolTip(trAudience("Volume","مستوى الصوت"));row->addWidget(volume_);connect(volume_,&QSlider::valueChanged,this,&AudienceSurface::volumeRequested);auto full=button("full",trAudience("Full screen / restore","ملء الشاشة / استعادة"));
 connect(previous,&QToolButton::clicked,this,&AudienceSurface::previousRequested);connect(playButton_,&QToolButton::clicked,this,&AudienceSurface::playPauseRequested);connect(next,&QToolButton::clicked,this,&AudienceSurface::nextRequested);connect(stop,&QToolButton::clicked,this,&AudienceSurface::stopRequested);connect(full,&QToolButton::clicked,this,&AudienceSurface::fullscreenRequested);
 connect(seek_,&QSlider::sliderReleased,this,[this]{if(!radio_&&durationSeconds_>0)Q_EMIT seekRequested(quint64(durationSeconds_*seek_->value()/10000));revealControls();});
 connect(seek_,&QSlider::sliderMoved,this,[this](int value){time_->setText(videoTime(durationSeconds_*value/10000)+" / "+videoTime(durationSeconds_));});
 // Keyboard changes seek on release of the slider action, while dragging emits once.
 connect(seek_,&QSlider::actionTriggered,this,[this](int action){if(!seek_->isSliderDown()&&action!=QAbstractSlider::SliderMove&&!radio_&&durationSeconds_>0)Q_EMIT seekRequested(quint64(durationSeconds_*seek_->sliderPosition()/10000));});
 for(auto widget:controls_->findChildren<QWidget*>()){widget->setMouseTracking(true);widget->installEventFilter(this);}controls_->setMouseTracking(true);controls_->installEventFilter(this);
 hideControls_.setSingleShot(true);hideControls_.setInterval(3000);connect(&hideControls_,&QTimer::timeout,this,[this]{if(seek_->isSliderDown()||volume_->isSliderDown()||QApplication::mouseButtons()!=Qt::NoButton||(keyboardControls_&&controls_->isAncestorOf(QApplication::focusWidget()))){hideControls_.start();return;}fade_->setStartValue(opacity_->opacity());fade_->setEndValue(0.);fade_->start();});
 controls_->hide();setTimeline(0,0);
}
void AudienceSurface::setTimeline(qint64 positionSeconds,qint64 durationSeconds){
 positionSeconds_=qMax<qint64>(0,positionSeconds);durationSeconds_=qMax<qint64>(0,durationSeconds);if(!controls_)return;
 seek_->setVisible(!radio_);seek_->setEnabled(!radio_&&durationSeconds_>0);if(radio_){time_->setText(reconnecting_?trAudience("Reconnecting…","جارٍ الاتصال…"):playing_?trAudience("● LIVE","● بث مباشر"):trAudience("Paused","متوقف مؤقتًا"));return;}if(!seek_->isSliderDown()){seek_->setValue(durationSeconds_?int(qMin(positionSeconds_,durationSeconds_)*10000/durationSeconds_):0);time_->setText(videoTime(positionSeconds_)+" / "+videoTime(durationSeconds_));}
}
void AudienceSurface::setVolume(int volume){if(volume_&&!volume_->isSliderDown()){QSignalBlocker blocker(volume_);volume_->setValue(volume);}}
void AudienceSurface::setNavigation(bool previous,bool next){if(!controls_)return;controls_->findChild<QToolButton*>("video_previous")->setEnabled(previous);controls_->findChild<QToolButton*>("video_next")->setEnabled(next);}
void AudienceSurface::revealControls(){if(!controls_||!controlsAvailable())return;fade_->stop();opacity_->setOpacity(1);controls_->show();controls_->raise();setCursor(Qt::ArrowCursor);hideControls_.start();}
void AudienceSurface::syncControls(bool wasVideo){if(!controls_)return;if(!controlsAvailable()){fade_->stop();controls_->hide();hideControls_.stop();setCursor(Qt::ArrowCursor);}Q_UNUSED(wasVideo);}
bool AudienceSurface::eventFilter(QObject *object,QEvent *event){
 if(event->type()==QEvent::MouseButtonPress)keyboardControls_=false;
 if(event->type()==QEvent::KeyPress)keyboardControls_=true;


 if(event->type()==QEvent::KeyPress)revealControls();
 if(event->type()==QEvent::MouseMove||event->type()==QEvent::MouseButtonPress||event->type()==QEvent::MouseButtonRelease){
  const auto widget=qobject_cast<QWidget*>(object);const auto mouse=static_cast<QMouseEvent*>(event);
  if(controls_&&widget&&controls_->geometry().adjusted(-16,-20,16,18).contains(widget->mapTo(this,mouse->position().toPoint())))revealControls();
 }
 if(object==seek_&&seek_->isEnabled()){
  const auto type=event->type();
  if(type==QEvent::MouseButtonPress||type==QEvent::MouseMove||type==QEvent::MouseButtonRelease){
   const auto mouse=static_cast<QMouseEvent*>(event);
   if((type==QEvent::MouseButtonPress&&mouse->button()==Qt::LeftButton)||seek_->isSliderDown()){
    if(type==QEvent::MouseButtonPress)seek_->setSliderDown(true);
    const int handle=seek_->style()->pixelMetric(QStyle::PM_SliderLength,nullptr,seek_);
    seek_->setSliderPosition(QStyle::sliderValueFromPosition(0,10000,int(mouse->position().x())-handle/2,qMax(1,seek_->width()-handle)));
    if(type==QEvent::MouseButtonRelease)seek_->setSliderDown(false);
    event->accept();return true;
   }
  }
 }
 return QWidget::eventFilter(object,event);
}
void AudienceSurface::mouseMoveEvent(QMouseEvent *event){QWidget::mouseMoveEvent(event);}
void AudienceSurface::resizeEvent(QResizeEvent *event){QWidget::resizeEvent(event);if(controls_){const int w=qMin(760,width()-32);controls_->setGeometry((width()-w)/2,height()-112,w,94);}}
void AudienceSurface::setPlayback(bool playing,bool radio,bool reconnecting,const QString &title,const QString &countdown,const QVector<float> &spectrum){
 const double elapsed=clock_.isValid()?qMin<qint64>(clock_.restart(),150)/1000.:0.;if(!clock_.isValid())clock_.start();
 const bool mediaChanged=title_!=title||radio_!=radio;
 playing_=playing&&!prayer_&&!blocked_;radio_=radio;reconnecting_=reconnecting;title_=title;countdown_=countdown;
 Q_UNUSED(mediaChanged);
 if(playButton_ && playButton_->property("playing").toBool()!=playing_){playButton_->setProperty("playing",playing_);playButton_->setIcon(videoIcon(playing_?"pause":"play"));}
 if(playing_&&!reconnecting_){angle_=std::fmod(angle_+elapsed*24.,360.);phase_+=elapsed;}
 spectrum_=playing_&&!reconnecting_?spectrum:QVector<float>();if(isVisible())update();
}
void AudienceSurface::paintEvent(QPaintEvent*){
 QPainter p(this);p.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);p.fillRect(rect(),QColor("#102f28"));
 if(prayer_&&controls_){
  static const QImage defaultPrayer(QStringLiteral(":/nasaq/adhan-default.png"));
  const bool custom=!prayerImage_.isNull();const QImage &card=custom?prayerImage_:defaultPrayer;
  p.fillRect(rect(),QColor("#faf4e7"));
  const auto fitted=card.size().scaled(size(),Qt::KeepAspectRatio);const QRectF box((width()-fitted.width())/2.,(height()-fitted.height())/2.,fitted.width(),fitted.height());p.drawImage(box,card);
  if(!custom){
   p.setPen(QColor("#596039"));QFont textFont(QStringLiteral("Segoe UI"));textFont.setPixelSize(qMax(14,int(box.width()*.027)));p.setFont(textFont);
   QTextOption arabicText(Qt::AlignCenter);arabicText.setTextDirection(Qt::RightToLeft);p.drawText(QRectF(box.x()+12,box.y()+box.height()*.72,box.width()-24,box.height()*.09),QStringLiteral("الموسيقى متوقفة حالياً احتراماً للآذان."),arabicText);
   textFont.setPixelSize(qMax(11,int(box.width()*.020)));p.setFont(textFont);
   p.drawText(QRectF(box.x()+12,box.y()+box.height()*.81,box.width()-24,box.height()*.07),Qt::AlignCenter,QStringLiteral("Music is currently paused out of respect for the Adhan."));
  }
  const QRectF badge(qMax(12.,width()/2.-240),height()-65,qMin(480,width()-24),51);p.setPen(Qt::NoPen);p.setBrush(QColor(250,248,238,235));p.drawRoundedRect(badge,12,12);
  QFont timerFont(QStringLiteral("Segoe UI"));timerFont.setPixelSize(qBound(12,width()/65,20));p.setFont(timerFont);p.setPen(QColor("#596039"));p.drawText(badge,Qt::AlignCenter,countdown_);return;
 }
 const QImage &picture=prayer_?prayerImage_:(!blocked_&&!frame_.isNull()?frame_:waiting_);
 const bool turntable=picture.isNull()&&!prayer_;
 if(!picture.isNull()){
  const auto size=picture.size().scaled(this->size(),crop_&&!prayer_&&!blocked_?Qt::KeepAspectRatioByExpanding:Qt::KeepAspectRatio);
  p.drawImage(QRect(QPoint((width()-size.width())/2,(height()-size.height())/2),size),picture);
 }else if(turntable){
  p.fillRect(rect(),QColor("#f5f4eb"));const double side=qMin(height()-40.,width()-qMin(width()-24.,245*qBound(.55,width()/1100.,1.6))-54.);
  PaintNasaqTurntable(p,QRectF(14,(height()-side)/2,side,side),angle_,tonearmAngle());
 }else{
  p.setPen(QColor("#e4e9d5"));QFont f=font();f.setPixelSize(qBound(18,width()/24,48));f.setBold(true);p.setFont(f);
  p.drawText(rect().adjusted(24,24,-24,-70),Qt::AlignCenter|Qt::TextWordWrap,trAudience("Paused for prayer","توقف مؤقت للصلاة"));
 }
 // One compact, responsive overlay rail. No playback controls on the audience surface.
 const double scale=qBound(.55,width()/1100.,1.6);const int rail=qMin(width()-24,int(245*scale));const int x=width()-rail-18;
 QFont f=font();f.setPixelSize(qMax(11,int(17*scale)));p.setFont(f);
 if(radio_&&!prayer_){
  const QColor red=reconnecting_?QColor("#a27632"):playing_?QColor("#c4313b"):QColor("#7d8276");
  p.setPen(Qt::NoPen);p.setBrush(QColor(250,249,241,235));p.drawRoundedRect(QRectF(x,18,rail,43*scale),12,12);
  p.setPen(red);f.setBold(true);p.setFont(f);
  p.drawText(QRectF(x+8,18,rail-55*scale,43*scale),Qt::AlignCenter,reconnecting_?trAudience("Reconnecting","إعادة الاتصال"):playing_?trAudience("LIVE","بث مباشر"):trAudience("Paused","متوقف مؤقتًا"));
  const QPointF dot(width()-40*scale,18+21*scale);p.setBrush(red);p.setPen(Qt::NoPen);p.drawEllipse(dot,3.5*scale,3.5*scale);
  for(int i=0;i<3;++i){QColor wave=red;wave.setAlphaF(playing_&&!reconnecting_?.25+.65*(.5+.5*std::sin(phase_*4-i)): .3);p.setPen(QPen(wave,1.6*scale));p.setBrush(Qt::NoBrush);double r=(8+i*5)*scale;p.drawArc(QRectF(dot.x()-r,dot.y()-r,r*2,r*2),-60*16,120*16);}
 }
 if(turntable){
  f.setBold(false);f.setPixelSize(qMax(12,int(20*scale)));p.setFont(f);p.setPen(QColor("#173f34"));
  p.drawText(QRectF(x,82*scale,rail,75*scale),Qt::AlignCenter|Qt::TextWordWrap,QFontMetrics(p.font()).elidedText(title_.isEmpty()?QStringLiteral("NASAQ | نسق"):title_,Qt::ElideRight,rail*2));
  if(!cover_.isNull()&&!radio_){
   const double size=qMax(24.,qMin(rail-24.,height()-330*scale));
   p.save();p.translate(x+rail/2.,170*scale+size/2);p.rotate(-4);
   const QRectF sleeve(-size/2,-size/2,size,size);
   p.setPen(Qt::NoPen);for(int i=12;i>=1;--i){p.setBrush(QColor(24,38,25,3));p.drawRoundedRect(sleeve.adjusted(-i/2.,-i/2.,i/2.,i/2.).translated(5,8),2,2);}
   p.setBrush(QColor("#fcfaf3"));p.drawRect(sleeve);p.setBrush(QColor("#c9c5b5"));p.drawRect(QRectF(sleeve.left(),sleeve.top(),3,size));
   const auto fitted=cover_.size().scaled(QSize(int(size-12),int(size-12)),Qt::KeepAspectRatio);
   p.drawImage(QRectF(-fitted.width()/2.,-fitted.height()/2.,fitted.width(),fitted.height()),cover_);
   p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor(255,255,255,150),1));p.drawRect(sleeve.adjusted(1,1,-1,-1));p.restore();
  }
 }
 if(!prayer_&&!videoVisible()&&(turntable||!spectrum_.isEmpty())){
  const QRectF bars(x,height()-150*scale,rail,63*scale);p.setPen(Qt::NoPen);
  if(!turntable){p.setBrush(QColor(245,244,235,205));p.drawRoundedRect(bars.adjusted(-8,-8,8,8),10,10);}
  for(int i=0;i<24;++i){const float value=i<spectrum_.size()?qBound(0.f,spectrum_[i],1.f):0;const double h=2+value*(bars.height()-2);p.setBrush(i%3?QColor("#82916b"):QColor("#244f3d"));p.drawRoundedRect(QRectF(bars.x()+i*bars.width()/24,bars.bottom()-h,bars.width()/24*.58,h),2,2);}
 }
 if(!videoVisible()&&!countdown_.isEmpty()){
  f.setBold(false);f.setPixelSize(qMax(10,int(12*scale)));p.setFont(f);const QRectF badge(x-12,height()-65*scale,rail+12,51*scale);
  p.setPen(Qt::NoPen);p.setBrush(QColor(249,248,240,235));p.drawRoundedRect(badge,10,10);p.setPen(prayer_?QColor("#ad4245"):QColor("#52664d"));
  p.drawText(badge.adjusted(8,4,-8,-4),Qt::AlignCenter|Qt::TextWordWrap,countdown_);
 }
}
void AudienceSurface::contextMenuEvent(QContextMenuEvent *e){e->accept();}
void AudienceSurface::keyPressEvent(QKeyEvent *e){revealControls();QWidget::keyPressEvent(e);}

QString AudienceDisplay::screenKey(QScreen *s){
 if(!s)return {};
 return s->name()+"|"+s->manufacturer()+"|"+s->model()+"|"+s->serialNumber();
}
AudienceDisplay::AudienceDisplay(Store &store,QMainWindow *main):QObject(main),store_(store),main_(main){
 qApp->installEventFilter(this);
 floating_=std::make_unique<QMainWindow>();floating_->setObjectName("nasaqFloatingPlayer");floating_->setWindowTitle(trAudience("NASAQ | Media display","نسق NASAQ | عرض الوسائط"));floating_->setMinimumSize(480,320);floating_->resize(1000,680);
 floating_->setWindowIcon(main->windowIcon());preview_=new AudienceSurface(floating_.get());preview_->setCursor(Qt::ArrowCursor);floating_->setCentralWidget(preview_);floating_->installEventFilter(this);
 preview_->enableVideoControls();
 connect(preview_,&AudienceSurface::playPauseRequested,this,&AudienceDisplay::playPauseRequested);
 connect(preview_,&AudienceSurface::stopRequested,this,&AudienceDisplay::stopRequested);
 connect(preview_,&AudienceSurface::previousRequested,this,&AudienceDisplay::previousRequested);
 connect(preview_,&AudienceSurface::nextRequested,this,&AudienceDisplay::nextRequested);
 connect(preview_,&AudienceSurface::seekRequested,this,&AudienceDisplay::seekRequested);
 connect(preview_,&AudienceSurface::volumeRequested,this,&AudienceDisplay::volumeRequested);
 connect(preview_,&AudienceSurface::fullscreenRequested,this,[this]{if(floating_->isFullScreen())floating_->showNormal();else floating_->showFullScreen();preview_->revealControls();});
 QSettings geometry;floating_->restoreGeometry(geometry.value("Audience/floatingGeometry").toByteArray());
 audience_=std::make_unique<AudienceSurface>();
 audience_->setWindowFlags(Qt::Window|Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus|Qt::WindowStaysOnTopHint);
 audience_->setAttribute(Qt::WA_ShowWithoutActivating);audience_->setWindowTitle("NASAQ Audience");
 chosen_=store_.get("audience_screen");crop_=store_.get("audience_crop")=="1";
 enabled_=store_.get("audience_restore")=="1"&&!chosen_.isEmpty();reloadImages();
 auto menu=main->menuBar()->addMenu(trAudience("Display and screens","العرض والشاشات"));menu->setObjectName("nasaqAudienceMenu");
 menu->addAction(NasaqLineIcon(QStringLiteral("nasaq-display")),trAudience("Audience screen settings","إعدادات شاشة الجمهور"),this,&AudienceDisplay::settings);
 menu->addAction(trAudience("Show floating player","إظهار نافذة العرض العائمة"),this,&AudienceDisplay::showFloating);
 auto full=menu->addAction(trAudience("Floating window full screen","ملء الشاشة للنافذة العائمة"));connect(full,&QAction::triggered,this,[this]{if(floating_->isFullScreen())floating_->showNormal();else floating_->showFullScreen();});
 auto top=menu->addAction(trAudience("Keep floating player on top","إبقاء النافذة العائمة في المقدمة"));top->setCheckable(true);connect(top,&QAction::toggled,this,[this](bool checked){floating_->setWindowFlag(Qt::WindowStaysOnTopHint,checked);showFloating();});
 menu->addAction(trAudience("Hide audience screen","إخفاء شاشة الجمهور"),this,&AudienceDisplay::stopExternal);
 connect(qApp,&QGuiApplication::screenAdded,this,[this](QScreen*){reconcileScreens();});
 connect(qApp,&QGuiApplication::screenRemoved,this,[this](QScreen *s){
  if(s==target_){audience_->hide();target_=nullptr;Q_EMIT safetyStopRequested();}
  QTimer::singleShot(0,this,[this]{reconcileScreens();});
 });
 // Re-check geometry and the operator's screen after display reconfiguration.
 auto check=new QTimer(this);connect(check,&QTimer::timeout,this,[this]{reconcileScreens();});check->start(1000);
 QTimer::singleShot(0,this,[this]{reconcileScreens();});
}
AudienceDisplay::~AudienceDisplay()=default;
void AudienceDisplay::beginTrack(const QString &key){mediaKey_=key;preview_->setAlbumCover({});audience_->setAlbumCover({});}
void AudienceDisplay::setAlbumCover(const QString &key,const QImage &cover,bool genuine){
 if(key.isEmpty()||key!=mediaKey_)return;
 const auto image=genuine?cover.scaled(1000,1000,Qt::KeepAspectRatio,Qt::SmoothTransformation):QImage();
 preview_->setAlbumCover(image);audience_->setAlbumCover(image);
}
QString AudienceDisplay::prayerCaption(Store &store,const QDateTime &now){
 const bool enabled=store.get("prayer_pause_enabled")!="0";
 if(!enabled){
  auto q=store.query("SELECT due FROM events WHERE kind IN ('prayer','adhan') AND id NOT LIKE 'prayer-early:%' AND status='pending' AND due>=? ORDER BY due LIMIT 1",{now.toMSecsSinceEpoch()});
  if(!q.next())return {};
  return trAudience("Next adhan: ","موعد الأذان القادم: ")+QDateTime::fromMSecsSinceEpoch(q.value(0).toLongLong(),QTimeZone::UTC).toLocalTime().toString("HH:mm");
 }
 const auto hold=QDateTime::fromString(store.get("prayer_hold_until"),Qt::ISODate);
 if(hold>now)return QStringLiteral("استئناف التشغيل / Resuming in  ")+videoTime(now.secsTo(hold));
 const auto next=PrayerService::nextPause(store,now);if(!next.isValid())return {};
 const auto seconds=qMax<qint64>(0,now.secsTo(next));
 return trAudience("Pause in: ","متبقي للتوقف: ")+QString("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0')).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
}
bool AudienceDisplay::eventFilter(QObject *object,QEvent *event){
 if(event->type()==QEvent::KeyPress){
  const auto key=static_cast<QKeyEvent*>(event);const auto widget=qobject_cast<QWidget*>(object);
  if(widget && (widget->window()==main_ || widget->window()==floating_.get()) && !QApplication::activeModalWidget() && key->key()==Qt::Key_Space && key->modifiers()==Qt::NoModifier){
   bool editing=false;
   for(auto focus=QApplication::focusWidget();focus;focus=focus->parentWidget()){
    if(qobject_cast<QLineEdit*>(focus)||qobject_cast<QTextEdit*>(focus)||qobject_cast<QPlainTextEdit*>(focus)||qobject_cast<QAbstractSpinBox*>(focus)){editing=true;break;}
   }
   if(!editing&&!qobject_cast<QAbstractButton*>(widget)){if(widget->window()==floating_.get())preview_->revealControls();if(!key->isAutoRepeat())Q_EMIT playPauseRequested();key->accept();return true;}
  }
 }
 if(object==floating_.get()&&event->type()==QEvent::KeyPress&&static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape&&floating_->isFullScreen()){floating_->showNormal();return true;}
 if(object==floating_.get()&&event->type()==QEvent::Close){QSettings settings;settings.setValue("Audience/floatingGeometry",floating_->saveGeometry());userHidden_=true;floating_->hide();event->ignore();return true;}
 return QObject::eventFilter(object,event);
}
void AudienceDisplay::showFloating(){
 userHidden_=false;
 bool visibleScreen=false;for(auto screen:QGuiApplication::screens())if(screen->availableGeometry().intersects(floating_->frameGeometry()))visibleScreen=true;
 if(!visibleScreen){floating_->resize(960,640);floating_->move(main_->screen()->availableGeometry().center()-QPoint(480,320));}
 floating_->show();floating_->raise();floating_->activateWindow();
}
void AudienceDisplay::manualMediaRequested(const QUrl &url){
 if(NasaqLocalVideo(url) && !externalActive())showFloating();
}
void AudienceDisplay::setTimeline(qint64 positionSeconds,qint64 durationSeconds){preview_->setTimeline(positionSeconds,durationSeconds);audience_->setTimeline(positionSeconds,durationSeconds);}
void AudienceDisplay::setPlayback(bool playing,bool radio,bool reconnecting,const QString &title,const QString &countdown,const QVector<float> &spectrum){
 preview_->setPlayback(playing,radio,reconnecting,title,countdown,spectrum);audience_->setPlayback(playing,radio,reconnecting,title,countdown,spectrum);
 if(playing&&!wasPlaying_&&!userHidden_&&!externalActive()&&!floating_->isVisible())showFloating();wasPlaying_=playing;
}
void AudienceDisplay::updateLanguage(){
 floating_->setWindowTitle(trAudience("NASAQ | Media display","نسق NASAQ | عرض الوسائط"));
 if(auto menu=main_->findChild<QMenu*>("nasaqAudienceMenu")){
  menu->setTitle(trAudience("Display and screens","العرض والشاشات"));
  menu->actions().first()->setText(trAudience("Audience screen settings","إعدادات شاشة الجمهور"));
  menu->actions().at(1)->setText(trAudience("Show floating player","إظهار نافذة العرض العائمة"));
  menu->actions().at(2)->setText(trAudience("Floating window full screen","ملء الشاشة للنافذة العائمة"));
  menu->actions().at(3)->setText(trAudience("Keep floating player on top","إبقاء النافذة العائمة في المقدمة"));
  menu->actions().last()->setText(trAudience("Hide audience screen","إخفاء شاشة الجمهور"));
 }repaintSurfaces();
}
bool AudienceDisplay::externalActive() const{return audience_->isVisible()&&target_;}
void AudienceDisplay::stopExternal(){enabled_=false;audience_->hide();target_=nullptr;store_.set("audience_restore","0");}
void AudienceDisplay::reconcileScreens(){
 if(!enabled_){audience_->hide();return;}
 QScreen *match=nullptr;for(auto s:QGuiApplication::screens())if(screenKey(s)==chosen_)match=s;
 // Never migrate the audience window onto the operator's monitor when unplugged.
 if(!match||match==main_->screen()||QGuiApplication::screens().size()<2){
  if(audience_->isVisible()){audience_->hide();Q_EMIT safetyStopRequested();}target_=nullptr;return;
 }
 if(target_!=match||!audience_->isVisible()){
  audience_->winId();audience_->windowHandle()->setScreen(match);audience_->setGeometry(match->geometry());target_=match;
  audience_->showFullScreen();
 }else if(audience_->geometry()!=match->geometry())audience_->setGeometry(match->geometry());
}
void AudienceDisplay::updateFrame(const QImage &frame,int generation,bool prayer,bool blocked){
 if(generation!=generation_){frame_={};generation_=generation;if(generation>=0&&!userHidden_&&!externalActive()&&!floating_->isVisible())showFloating();}
 if(!frame.isNull())frame_=frame;
 prayer_=prayer;blocked_=blocked;repaintSurfaces();
}
void AudienceDisplay::repaintSurfaces(){
 preview_->present(frame_,prayer_,blocked_,crop_,waiting_,prayerImage_);
 audience_->present(frame_,prayer_,blocked_,crop_,waiting_,prayerImage_);
}
void AudienceDisplay::reloadImages(){waiting_=loadImage(store_.get("audience_waiting_image"));prayerImage_=loadImage(store_.get("audience_prayer_image"));repaintSurfaces();}
QString AudienceDisplay::importImage(const QString &source,const QString &directory){
 const auto image=loadImage(source);if(image.isNull())throw std::runtime_error("Invalid image (maximum 20 MB / 16000 pixels)");
 QByteArray data;QBuffer buffer(&data);buffer.open(QIODevice::WriteOnly);if(!image.save(&buffer,"PNG"))throw std::runtime_error("Cannot encode image");
 if(!QDir().mkpath(directory))throw std::runtime_error("Cannot create image folder");
 const auto path=QDir(directory).filePath(QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex())+".png");
 QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())throw std::runtime_error("Cannot save image");return path;
}
void AudienceDisplay::settings(){
 QDialog d(main_);d.setObjectName("audienceSettings");d.setWindowTitle(trAudience("Display and screens","العرض والشاشات"));d.resize(640,680);
 QVBoxLayout outer(&d);QScrollArea scroll;scroll.setWidgetResizable(true);scroll.setFrameShape(QFrame::NoFrame);auto content=new QWidget;QFormLayout form(content);scroll.setWidget(content);outer.addWidget(&scroll);
 QLabel heading(trAudience("Audience screen","شاشة الجمهور"));heading.setObjectName("title");form.addRow(&heading);
 QLabel note(trAudience("Use Windows Extend mode. The audience sees only the content; all controls stay on this computer. Audio uses NASAQ's current audio output. Screen or audio-device changes stop playback for safety.","استخدم وضع توسيع الشاشات في ويندوز. تظهر أدوات التحكم على الكمبيوتر فقط. يستخدم الفيديو مخرج الصوت الحالي لنسق. يتوقف التشغيل عند تغيير الشاشة أو جهاز الصوت للحماية."));note.setWordWrap(true);form.addRow(&note);
 QCheckBox enable(trAudience("Enable audience display","تفعيل شاشة الجمهور"));enable.setChecked(enabled_);form.addRow(&enable);
 QComboBox screens;screens.setObjectName("audienceScreenSelector");screens.setPlaceholderText(trAudience("Connect an external screen","صِل شاشة خارجية"));for(auto s:QGuiApplication::screens())if(s!=main_->screen())screens.addItem(QString::number(QGuiApplication::screens().indexOf(s)+1)+" — "+s->name()+QString(" (%1 × %2)").arg(s->size().width()).arg(s->size().height()),screenKey(s));
 screens.setCurrentIndex(qMax(0,screens.findData(chosen_)));form.addRow(trAudience("External screen","الشاشة الخارجية"),&screens);enable.setEnabled(screens.count()>0);
 if(screens.count()==0){enable.setChecked(false);note.setText(note.text()+"\n"+trAudience("No external screen detected. Connect one in Extend mode; the local preview remains available.","لم تُكتشف شاشة خارجية. صِل شاشة بوضع التوسيع؛ تبقى المعاينة المحلية متاحة."));}
 QPushButton identify(trAudience("Identify screens","تحديد الشاشات"));form.addRow(&identify);connect(&identify,&QPushButton::clicked,&d,[]{int n=0;for(auto s:QGuiApplication::screens()){auto label=new QLabel(QString::number(++n));label->setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowDoesNotAcceptFocus);label->setAttribute(Qt::WA_ShowWithoutActivating);label->setAlignment(Qt::AlignCenter);label->setStyleSheet("background:#153e34;color:#f8f7f1;font-size:70px;padding:24px");label->setGeometry(QRect(s->geometry().center()-QPoint(80,80),QSize(160,160)));label->show();QTimer::singleShot(2000,label,&QObject::deleteLater);}});
 QLabel presentation(trAudience("Picture layout","طريقة عرض الصورة"));presentation.setObjectName("title");form.addRow(&presentation);
 QCheckBox crop(trAudience("Fill screen by cropping (default: fit without distortion)","ملء الشاشة بالقص (الافتراضي: احتواء دون تشويه)"));crop.setChecked(crop_);form.addRow(&crop);
 QCheckBox restore(trAudience("Restore display at startup, without starting playback","استعادة العرض عند فتح نسق دون بدء التشغيل"));restore.setChecked(store_.get("audience_restore")=="1");form.addRow(&restore);
 QPushButton output(trAudience("Audio output settings","إعدادات مخرج الصوت"));form.addRow(&output);connect(&output,&QPushButton::clicked,&d,[&]{d.reject();Q_EMIT audioSettingsRequested();});
 QLabel stills(trAudience("Prayer and waiting screens","الصلاة والانتظار"));stills.setObjectName("title");form.addRow(&stills);
 QLabel stillHelp(trAudience("A still image replaces the video throughout the prayer pause, including the early pause. Playback controls remain on this computer.","تظهر صورة ثابتة بدل الفيديو طوال توقف الصلاة، بما فيه التوقف المبكر. تبقى أدوات التحكم على هذا الكمبيوتر."));stillHelp.setWordWrap(true);form.addRow(&stillHelp);
 QString prayerPath=store_.get("audience_prayer_image"),waitingPath=store_.get("audience_waiting_image");
 QPushButton prayer(trAudience("Choose prayer image","اختيار صورة الصلاة")),waiting(trAudience("Choose audio / radio image","اختيار صورة للصوت والراديو")),reset(trAudience("Restore vinyl and default prayer image","استعادة الفينيل وصورة الصلاة الافتراضية"));form.addRow(&prayer);form.addRow(&waiting);form.addRow(&reset);
 QLabel feedback;feedback.setObjectName("audienceSaveFeedback");feedback.setWordWrap(true);form.addRow(&feedback);
 auto choose=[&](QString &path){const auto file=QFileDialog::getOpenFileName(&d,trAudience("Choose image","اختيار صورة"),{},"Images (*.png *.jpg *.jpeg *.webp)");if(file.isEmpty())return;try{path=importImage(file,QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/CAM/audience");feedback.setText(trAudience("Image ready. Save to apply.","الصورة جاهزة. احفظ لتطبيقها."));}catch(const std::exception&){feedback.setText(trAudience("Cannot load image. Choose a valid image smaller than 20 MB.","تعذر تحميل الصورة. اختر صورة صالحة أصغر من 20 ميغابايت."));}};
 connect(&prayer,&QPushButton::clicked,&d,[&]{choose(prayerPath);});connect(&waiting,&QPushButton::clicked,&d,[&]{choose(waitingPath);});connect(&reset,&QPushButton::clicked,&d,[&]{prayerPath.clear();waitingPath.clear();feedback.setText(trAudience("Default images selected.","تم اختيار الصور الافتراضية."));});
 QDialogButtonBox buttons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);outer.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);connect(&buttons,&QDialogButtonBox::accepted,&d,[&]{
  const bool nextEnabled=enable.isChecked()&&screens.count()>0;
  const QString nextScreen=screens.count()>0?screens.currentData().toString():chosen_;
  bool transaction=false;
  try{
   store_.exec("BEGIN IMMEDIATE");transaction=true;
   store_.set("audience_screen",nextScreen);store_.set("audience_crop",crop.isChecked()?"1":"0");store_.set("audience_restore",nextEnabled&&restore.isChecked()?"1":"0");store_.set("audience_prayer_image",prayerPath);store_.set("audience_waiting_image",waitingPath);
   store_.exec("COMMIT");transaction=false;
  }catch(const std::exception&){
   if(transaction){try{store_.exec("ROLLBACK");}catch(const std::exception&){}}
   feedback.setText(trAudience("Could not save display settings. Your previous settings are unchanged. Check available disk space and try again.","تعذر حفظ إعدادات العرض. لم تتغير إعداداتك السابقة. تحقق من المساحة المتاحة ثم أعد المحاولة."));
   scroll.ensureWidgetVisible(&feedback);return;
  }
  enabled_=nextEnabled;chosen_=nextScreen;crop_=crop.isChecked();
  reloadImages();reconcileScreens();d.accept();
 });ApplyNasaqFormDesign(&d);d.exec();
}
