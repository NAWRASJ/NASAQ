// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QtWidgets>
#include "radios/radioviewcontainer.h"
#include "core/iconloader.h"
#include "nasaqtheme.h"
#include "nasaqbrand.h"
#include "livebadge.h"
#include "formdesign.h"
#include "widgets/stretchheaderview.h"
#include "widgets/fancytabwidget.h"
#include "playlist/playlistview.h"
#include "playlist/playlistsequence.h"
#include "playlist/playlistdelegates.h"
#include "playlist/liveradiorole.h"
#include "context/contextalbum.h"
#include "dialogs/aboutdialog.h"
class VisualPlaylistProbe : public PlaylistView { protected: void showEvent(QShowEvent *e) override { QTreeView::showEvent(e); } };
class DesignReview : public QObject {
 Q_OBJECT
 void save(QWidget &widget,const QString &name) {
  const auto dir=qEnvironmentVariable("NASAQ_DESIGN_EVIDENCE");if(dir.isEmpty())return;
  QVERIFY(QDir().mkpath(dir));QVERIFY(widget.grab().save(dir+QLatin1Char('/')+name+QStringLiteral(".png")));
 }
 private Q_SLOTS:
 void radioLayouts_data(){QTest::addColumn<QString>("language");QTest::addColumn<int>("width");
  QTest::newRow("en-compact")<<QStringLiteral("en")<<440;QTest::newRow("ar-compact")<<QStringLiteral("ar")<<440;
  QTest::newRow("en-wide")<<QStringLiteral("en")<<820;QTest::newRow("ar-wide")<<QStringLiteral("ar")<<820;
 }
 void radioLayouts(){QFETCH(QString,language);QFETCH(int,width);
  qApp->setProperty("nasaqLiveLanguage",language);qApp->setLayoutDirection(language==QStringLiteral("ar")?Qt::RightToLeft:Qt::LeftToRight);
  RadioViewContainer radio;radio.resize(width,740);radio.show();QTest::qWait(120);
  QCOMPARE(radio.width(),width);
  const auto list=radio.findChild<QListWidget*>(QStringLiteral("curatedStations"));QVERIFY(list);QCOMPARE(list->count(),27);QVERIFY(list->height()>=100);
  const auto pane=radio.findChild<QWidget*>(QStringLiteral("curatedRadio"));QVERIFY(pane);
  for(auto *button:pane->findChildren<QPushButton*>()){
   QVERIFY(button->isVisible());QVERIFY(pane->rect().contains(button->geometry()));
   QVERIFY2(button->width()>=button->fontMetrics().horizontalAdvance(button->text())+button->iconSize().width()+20,qPrintable(button->text()));
  }
  save(radio,QStringLiteral("radio-%1-%2").arg(language).arg(width));
 }
 void formsAndMenus_data(){QTest::addColumn<QString>("language");QTest::newRow("en")<<QStringLiteral("en");QTest::newRow("ar")<<QStringLiteral("ar");}
 void formsAndMenus(){QFETCH(QString,language);const bool ar=language==QStringLiteral("ar");qApp->setProperty("nasaqLiveLanguage",language);qApp->setLayoutDirection(ar?Qt::RightToLeft:Qt::LeftToRight);
  QDialog dialog;QVBoxLayout layout(&dialog);layout.setContentsMargins(24,24,24,24);layout.setSpacing(16);
  QLabel heading(ar?QStringLiteral("أوقات الأذان"):QStringLiteral("Adhan times"));heading.setObjectName(QStringLiteral("sectionHeading"));layout.addWidget(&heading);
  QLabel hint(ar?QStringLiteral("ضبط مواقيت الصلاة والتوقف التلقائي"):QStringLiteral("Prayer times and automatic music pauses"));hint.setObjectName(QStringLiteral("subtitle"));layout.addWidget(&hint);
  QFormLayout form;layout.addLayout(&form);QLineEdit city(ar?QStringLiteral("دبي"):QStringLiteral("Dubai"));QComboBox method;method.addItem(ar?QStringLiteral("الهيئة العامة للشؤون الإسلامية"):QStringLiteral("General Authority of Islamic Affairs"));QSpinBox minutes;minutes.setRange(1,60);minutes.setValue(7);
  form.addRow(ar?QStringLiteral("المدينة"):QStringLiteral("City"),&city);form.addRow(ar?QStringLiteral("جهة الحساب"):QStringLiteral("Calculation authority"),&method);form.addRow(ar?QStringLiteral("مدة التوقف (دقائق)"):QStringLiteral("Pause duration (minutes)"),&minutes);
  QCheckBox enabled(ar?QStringLiteral("إيقاف الموسيقى تلقائيًا عند الأذان"):QStringLiteral("Automatically pause music at adhan"));enabled.setChecked(true);layout.addWidget(&enabled);
  QPushButton saveButton(ar?QStringLiteral("حفظ الإعدادات"):QStringLiteral("Save settings"));saveButton.setObjectName(QStringLiteral("primary"));layout.addWidget(&saveButton);ApplyNasaqFormDesign(&dialog);dialog.resize(620,360);dialog.show();QTest::qWait(80);
  const auto before=city.geometry();city.setFocus();QTest::qWait(40);QCOMPARE(city.geometry(),before);QVERIFY(dialog.rect().contains(saveButton.geometry()));save(dialog,QStringLiteral("form-%1").arg(language));
  QMenu menu;menu.addAction(IconLoader::Load(QStringLiteral("document-open")),ar?QStringLiteral("فتح ملف صوتي"):QStringLiteral("Open audio"));menu.addAction(IconLoader::Load(QStringLiteral("document-new")),ar?QStringLiteral("قائمة تشغيل جديدة"):QStringLiteral("New playlist"));menu.addSeparator();menu.addAction(IconLoader::Load(QStringLiteral("configure")),ar?QStringLiteral("الإعدادات"):QStringLiteral("Settings"));menu.show();QTest::qWait(60);save(menu,QStringLiteral("menu-%1").arg(language));
 }
 void compactControls_data(){formsAndMenus_data();}
 void compactControls(){
  QFETCH(QString,language);const bool ar=language==QStringLiteral("ar");qApp->setLayoutDirection(ar?Qt::RightToLeft:Qt::LeftToRight);
  QWidget host;QVBoxLayout layout(&host);QSpinBox minutes;minutes.setRange(0,960);minutes.setValue(480);minutes.setSuffix(ar?QStringLiteral(" دقيقة عمل"):QStringLiteral(" business minutes"));layout.addWidget(&minutes);
  QTabBar tabs;tabs.setTabsClosable(true);tabs.setUsesScrollButtons(true);tabs.addTab(IconLoader::Load(QStringLiteral("star")),QStringLiteral("Playlist 1"));tabs.addTab(QStringLiteral("NASAQ QA Store313"));tabs.addTab(QStringLiteral("Classical music"));layout.addWidget(&tabs);host.resize(420,150);host.show();QTest::qWait(80);
  QStyleOptionSpinBox option;option.initFrom(&minutes);option.rect=minutes.rect();option.frame=true;option.buttonSymbols=QAbstractSpinBox::UpDownArrows;option.stepEnabled=QAbstractSpinBox::StepUpEnabled|QAbstractSpinBox::StepDownEnabled;
  const QRect up=minutes.style()->subControlRect(QStyle::CC_SpinBox,&option,QStyle::SC_SpinBoxUp,&minutes);
  QVERIFY(up.isValid());QTest::mouseClick(&minutes,Qt::LeftButton,Qt::NoModifier,up.center());QCOMPARE(minutes.value(),481);
  const QRect down=minutes.style()->subControlRect(QStyle::CC_SpinBox,&option,QStyle::SC_SpinBoxDown,&minutes);QTest::mouseClick(&minutes,Qt::LeftButton,Qt::NoModifier,down.center());QCOMPARE(minutes.value(),480);
  QSignalSpy closed(&tabs,&QTabBar::tabCloseRequested);QWidget *close=tabs.tabButton(0,QTabBar::RightSide);if(!close)close=tabs.tabButton(0,QTabBar::LeftSide);QVERIFY(close);QTest::mouseClick(close,Qt::LeftButton);QCOMPARE(closed.count(),1);
  save(host,QStringLiteral("controls-%1").arg(language));
 }
 void controlFamilies(){
  QWidget host;QGridLayout layout(&host);int row=0;
  QCheckBox check(QStringLiteral("Automatic pause"));QCheckBox disabled(QStringLiteral("Disabled"));disabled.setEnabled(false);QRadioButton radio(QStringLiteral("Selected option"));radio.setChecked(true);layout.addWidget(&check,row,0);layout.addWidget(&disabled,row++,1);layout.addWidget(&radio,row++,0);
  QComboBox combo;combo.addItems({QStringLiteral("Dubai"),QStringLiteral("Abu Dhabi")});layout.addWidget(&combo,row++,0,1,2);
  QDoubleSpinBox number;number.setValue(3.5);layout.addWidget(&number,row++,0,1,2);
  QDateTimeEdit date(QDateTime::currentDateTime());date.setCalendarPopup(true);layout.addWidget(&date,row++,0,1,2);
  QProgressBar progress;progress.setValue(65);layout.addWidget(&progress,row++,0,1,2);
  QSlider slider(Qt::Horizontal);slider.setValue(40);layout.addWidget(&slider,row++,0,1,2);
  QPushButton primary(QStringLiteral("Save settings"));primary.setObjectName(QStringLiteral("primary"));QPushButton cancel(QStringLiteral("Cancel"));layout.addWidget(&primary,row,0);layout.addWidget(&cancel,row++,1);
  host.resize(440,460);host.show();QTest::qWait(80);QTest::mouseClick(&check,Qt::LeftButton,Qt::NoModifier,QPoint(check.width()/2,check.height()/2));QVERIFY(check.isChecked());save(host,QStringLiteral("control-families"));
 }
 void hiddenColumnsDoNotReserveSpace(){
  for(auto direction:{Qt::LeftToRight,Qt::RightToLeft}){
    QTableView table;QStandardItemModel model(3,8);StretchHeaderView *header=new StretchHeaderView(Qt::Horizontal,&table);table.setHorizontalHeader(header);table.setModel(&model);table.setLayoutDirection(direction);table.resize(900,280);table.show();QTest::qWait(60);
    header->SetStretchEnabled(true);header->HideSection(6);header->HideSection(7);
    header->SetColumnWidth(6,0.15);header->SetColumnWidth(7,0.15);
    // Reproduce a saved legacy state with 30 percent reserved for hidden fields.
    QList<int> order,pixels,visible;QList<double> weights;
    for(int i=0;i<8;++i){order<<i;pixels<<100;weights<<(i<6?0.7/6:0.15);if(i<6)visible<<i;}
    QByteArray state;QDataStream stream(&state,QIODevice::WriteOnly);stream.setVersion(QDataStream::Qt_5_12);stream<<int(0x502C9510)<<true<<order<<pixels<<visible<<weights<<int(Qt::AscendingOrder)<<-1;
    QVERIFY(header->RestoreState(state));
    QVERIFY2(qAbs(header->length()-header->width())<=6,"Hidden columns leave a blank trailing region");
    QVERIFY(header->isSectionHidden(6));QVERIFY(header->isSectionHidden(7));
    const int before=header->sectionSize(0);header->SetColumnWidth(0,0.30);QVERIFY(header->RestoreState(header->SaveState()));QVERIFY(header->sectionSize(0)>before);
    table.resize(1100,280);QTest::qWait(40);QVERIFY(qAbs(header->length()-header->width())<=6);
    save(table,direction==Qt::RightToLeft?QStringLiteral("columns-ar"):QStringLiteral("columns-en"));
  }
 }
 void bilingualVectorBrand(){
  for(const QString language:{QStringLiteral("ar"),QStringLiteral("en")}){
   qApp->setProperty("nasaqLiveLanguage",language);qApp->setLayoutDirection(language==QStringLiteral("ar")?Qt::RightToLeft:Qt::LeftToRight);
   const auto title=NText(QStringLiteral("NASAQ / نسق"));QVERIFY(title.contains(QStringLiteral("NASAQ")));QVERIFY(title.contains(QStringLiteral("نسق")));
   QWidget host;QHBoxLayout layout(&host);NasaqBrandMark logo;QLabel name(title);name.setObjectName(QStringLiteral("brandHeading"));layout.addWidget(&logo);layout.addWidget(&name);host.resize(340,100);host.show();QTest::qWait(40);save(host,QStringLiteral("brand-%1").arg(language));
  }
 }
 void liveBadgeAnimation(){
  QWidget host;QVBoxLayout layout(&host);NasaqLiveBadge badge;layout.addWidget(&badge);host.show();badge.setLive(true,true,true);QTest::qWait(100);auto before=badge.grab().toImage();QTest::qWait(400);QVERIFY(before!=badge.grab().toImage());save(host,QStringLiteral("live-badge"));
  badge.setLive(true,false,true);auto paused=badge.grab().toImage();QTest::qWait(160);QCOMPARE(badge.grab().toImage(),paused);
  badge.setLive(false,false,true);QVERIFY(badge.isHidden());
 }
 void playingAndSelectedRowsDiffer(){
  VisualPlaylistProbe view;QStandardItemModel model(3,2);model.setHorizontalHeaderLabels({QStringLiteral("Title"),QStringLiteral("Length")});
  for(int i=0;i<3;++i){model.setData(model.index(i,0),QStringLiteral("Track %1").arg(i+1));model.setData(model.index(i,1),QStringLiteral("4:22"));}
  model.setData(model.index(2,0),true,Playlist::Role_IsCurrent);view.setModel(&model);view.setRootIsDecorated(false);view.resize(600,260);view.setColumnWidth(0,380);view.setColumnWidth(1,170);view.show();view.selectionModel()->select(model.index(1,0),QItemSelectionModel::Select|QItemSelectionModel::Rows);QTest::qWait(80);
  save(view,QStringLiteral("playing-selection"));
  auto image=view.viewport()->grab().toImage();const auto dpr=image.devicePixelRatio();
  const auto selected=view.visualRect(model.index(1,0));const auto playing=view.visualRect(model.index(2,0));
  auto pixel=[&](QRect rect){return image.pixelColor(qRound((rect.right()-25)*dpr),qRound(rect.center().y()*dpr));};
  QVERIFY(pixel(selected)!=pixel(playing));QCOMPARE(pixel(playing),QColor("#153e34"));
 }
 void radioColumnRendering(){
  QTreeView view;QStandardItemModel model(2,Playlist::ColumnCount);view.setModel(&model);LengthItemDelegate length(&view);PlaylistDelegateBase end(&view);
  const int lengthCol=static_cast<int>(Playlist::Column::Length),endCol=static_cast<int>(Playlist::Column::BusinessEnd);
  model.setData(model.index(0,lengthCol),qlonglong(262000000000));model.setData(model.index(0,endCol),QStringLiteral("09:10:59"));
  for(int col:{lengthCol,endCol}){model.setData(model.index(1,col),true,NasaqLiveRadioRole);model.setData(model.index(1,col),true,Playlist::Role_IsCurrent);}
  model.setData(model.index(1,endCol),QStringLiteral("بث مباشر"));
  auto render=[&]{QImage image(500,120,QImage::Format_ARGB32_Premultiplied);image.fill(QColor("#f8f7f2"));QPainter painter(&image);for(int row=0;row<2;++row){for(int col:{lengthCol,endCol}){QStyleOptionViewItem option;option.initFrom(&view);option.widget=&view;option.rect=QRect(col==lengthCol?250:0,row*60,250,60);option.displayAlignment=Qt::AlignCenter;model.setData(model.index(row,col),int(Qt::AlignCenter),Qt::TextAlignmentRole);if(row==1)painter.fillRect(option.rect,QColor("#153e34"));if(col==lengthCol)length.paint(&painter,option,model.index(row,col));else end.paint(&painter,option,model.index(row,col));}}return image;};
  view.viewport()->setProperty("nasaqRadioAnimating",true);const auto active=render();QTest::qWait(160);const auto next=render();QVERIFY(active.copy(250,60,250,60)!=next.copy(250,60,250,60));QCOMPARE(active.copy(0,0,500,60),next.copy(0,0,500,60));QCOMPARE(active.copy(0,60,250,60),next.copy(0,60,250,60));
  model.setData(model.index(1,lengthCol),true,Playlist::Role_IsPaused);const auto paused=render();QTest::qWait(100);QCOMPARE(paused,render());model.setData(model.index(1,lengthCol),false,Playlist::Role_IsPaused);view.viewport()->setProperty("nasaqRadioAnimating",false);const auto held=render();QTest::qWait(100);QCOMPARE(held,render());
  const auto dir=qEnvironmentVariable("NASAQ_DESIGN_EVIDENCE");if(!dir.isEmpty()){QDir().mkpath(dir);QVERIFY(active.save(dir+QStringLiteral("/radio-columns.png")));}
 }
 void sidebarUsesHeightAndBrand(){
  auto logo=QImage(QStringLiteral(":/pictures/nomusic.png"));QVERIFY(!logo.isNull());const auto c=logo.pixelColor(logo.width()/2,logo.height()/8);QVERIFY(qAbs(c.red()-25)<5);QVERIFY(qAbs(c.green()-63)<5);
  FancyTabWidget tabs;for(int i=0;i<8;++i)tabs.AddTab(new QWidget,QString::number(i),IconLoader::Load(QStringLiteral("radio")),QStringLiteral("Section %1").arg(i+1));tabs.LoadSettings(QStringLiteral("SidebarIsolated"));tabs.SetMode(FancyTabWidget::Mode::LargeSidebar);tabs.resize(620,780);tabs.show();QTest::qWait(120);auto bar=tabs.findChild<QTabBar*>();QVERIFY(bar);QVERIFY(qAbs(bar->tabRect(7).bottom()-tabs.height())<16);save(tabs,QStringLiteral("sidebar-filled"));
 }
 void playerMenus_data(){QTest::addColumn<QString>("language");QTest::newRow("ar")<<QStringLiteral("ar");QTest::newRow("en")<<QStringLiteral("en");}
 void playerMenus(){QFETCH(QString,language);const bool ar=language==QStringLiteral("ar");
  qApp->setProperty("nasaqLiveLanguage",language);qApp->setLayoutDirection(ar?Qt::RightToLeft:Qt::LeftToRight);
  PlaylistSequence sequence;sequence.SetShuffleMode(PlaylistSequence::ShuffleMode::Off);sequence.SetRepeatMode(PlaylistSequence::RepeatMode::Off);
  for(QMenu *menu:{sequence.shuffle_menu(),sequence.repeat_menu()}){
   for(auto *action:menu->actions()){
    QVERIFY(!action->text().isEmpty());
    QCOMPARE(action->text().contains(QRegularExpression(QStringLiteral("[\\x{0600}-\\x{06ff}]"))),ar);
   }
   menu->popup(QPoint(100,100));QTest::qWait(100);
   QVERIFY(!menu->mask().isEmpty());
   const auto actions=menu->actions();menu->setActiveAction(actions.at(1));
   save(*menu,QStringLiteral("%1-%2").arg(menu==sequence.shuffle_menu()?QStringLiteral("shuffle"):QStringLiteral("repeat"),language));
   actions.at(1)->trigger();QVERIFY(actions.at(1)->isChecked());QVERIFY(!actions.at(0)->isChecked());menu->close();
  }
  QCOMPARE(sequence.shuffle_mode(),PlaylistSequence::ShuffleMode::All);
  QCOMPARE(sequence.repeat_mode(),PlaylistSequence::RepeatMode::Track);
  NasaqTranslator translator;
  if(ar){QCOMPARE(translator.translate("PlaylistView","&Align text",nullptr,-1),QStringLiteral("محاذاة النص"));
   QCOMPARE(QCoreApplication::translate("ContextView","%1 songs").arg(12),QStringLiteral("عدد الأغاني: 12"));}
  QVERIFY(translator.translate("Song","Customer song title",nullptr,-1).isEmpty());
 }
 void aboutCredit(){qApp->setProperty("nasaqLiveLanguage",QStringLiteral("ar"));qApp->setLayoutDirection(Qt::RightToLeft);QCoreApplication::setApplicationVersion(QStringLiteral("0.3.16"));
  AboutDialog dialog;dialog.show();QTest::qWait(100);
  auto *title=dialog.findChild<QLabel*>(QStringLiteral("label_title"));auto *body=dialog.findChild<QLabel*>(QStringLiteral("label_text"));
  QVERIFY(title);QVERIFY(body);QVERIFY(title->text().contains(QStringLiteral("0.3.16")));QVERIFY(!title->text().contains(QStringLiteral("0.3.3")));
  QVERIFY(body->text().contains(QStringLiteral("Nawras Jarbouh")));QVERIFY(body->text().contains(QStringLiteral("GPL-3.0-or-later")));save(dialog,QStringLiteral("about-credit-ar"));
 }
 void contextUsesVinyl(){
  for(const QString name:{QStringLiteral("strawberry"),QStringLiteral("strawberry-grey"),QStringLiteral("strawberry-faded"),QStringLiteral("strawbs")}){
    QImage image(QStringLiteral(":/pictures/")+name+QStringLiteral(".png"));QVERIFY(!image.isNull());const auto color=image.pixelColor(image.width()/2,image.height()/8);QVERIFY(qAbs(color.red()-25)<5);QVERIFY(qAbs(color.green()-63)<5);
  }
  ContextAlbum album;album.resize(340,340);album.show();QTest::qWait(150);save(album,QStringLiteral("context-vinyl"));
 }
 void icons(){QWidget sheet;QGridLayout grid(&sheet);sheet.setStyleSheet(QStringLiteral("background:#f8f7f2;color:#153e34;"));
  const QStringList names={QStringLiteral("media-playback-start"),QStringLiteral("media-playback-pause"),QStringLiteral("media-playback-stop"),QStringLiteral("media-skip-forward"),QStringLiteral("library-music"),QStringLiteral("view-media-playlist"),QStringLiteral("nasaq-smart-playlist"),QStringLiteral("footsteps"),QStringLiteral("radio"),QStringLiteral("document-open"),QStringLiteral("device"),QStringLiteral("edit-find"),QStringLiteral("love"),QStringLiteral("configure"),QStringLiteral("applications-internet"),QStringLiteral("document-save")};
  int i=0;for(const auto &name:names){auto icon=IconLoader::Load(name);QVERIFY(!icon.isNull());for(int size:{16,24,48,96}){QVERIFY(!icon.pixmap(size,size).isNull());}auto *button=new QToolButton;button->setIcon(icon);button->setIconSize(QSize(32,32));button->setFixedSize(64,64);button->setToolTip(name);grid.addWidget(button,i/4,i%4);++i;}
  QVERIFY(IconLoader::Load(QStringLiteral("nasaq-smart-playlist")).pixmap(32,32).toImage()!=IconLoader::Load(QStringLiteral("view-media-playlist")).pixmap(32,32).toImage());sheet.show();QTest::qWait(50);save(sheet,QStringLiteral("icons"));
 }
};
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName(QStringLiteral("NASAQ-DesignReview"));app.setApplicationName(QStringLiteral("Isolated"));QTemporaryDir temp;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());Q_INIT_RESOURCE(data);ApplyNasaqTheme();DesignReview test;return QTest::qExec(&test,argc,argv);}
#include "designreview_test.moc"

