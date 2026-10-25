// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QtWidgets>
#include "nasaqtheme.h"
#include "ui_acoustidsettingspage.h"
#include "ui_appearancesettingspage.h"
#include "ui_backendsettingspage.h"
#include "ui_behavioursettingspage.h"
#include "ui_collectionsettingspage.h"
#include "ui_contextsettingspage.h"
#include "ui_coverssettingspage.h"
#include "ui_globalshortcutssettingspage.h"
#include "ui_lyricssettingspage.h"
#include "ui_networkproxysettingspage.h"
#include "ui_notificationssettingspage.h"
#include "ui_playlistsettingspage.h"
#include "ui_radiosettingspage.h"
#include "ui_scrobblersettingspage.h"
#include "ui_settingsdialog.h"
#include "ui_transcodersettingspage.h"
#include "ui_waveformsettingspage.h"
#include "ui_aboutdialog.h"
#include "ui_addstreamdialog.h"
#include "ui_console.h"
#include "ui_edittagdialog.h"
#include "ui_errordialog.h"
#include "ui_messagedialog.h"
#include "ui_saveplaylistsdialog.h"
#include "ui_trackselectiondialog.h"
#include "ui_userpassdialog.h"
class SettingsFormsReview:public QObject {
 Q_OBJECT

void capture(QWidget &form,const QString &name){
 form.resize(qMax(780,form.minimumSizeHint().width()),qMax(620,form.minimumSizeHint().height()));form.show();QTest::qWait(30);
 const QString dir=qEnvironmentVariable("NASAQ_DESIGN_EVIDENCE");QVERIFY(QDir().mkpath(dir));QVERIFY(form.grab().save(dir+QStringLiteral("/")+name+QStringLiteral(".png")));
 for(auto *tabs:form.findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i){tabs->setCurrentIndex(i);QTest::qWait(10);QVERIFY(form.grab().save(dir+QStringLiteral("/")+name+QStringLiteral("-tab")+QString::number(i)+QStringLiteral(".png")));}
}
private Q_SLOTS:
 void acoustidsettingspage(){QWidget form;Ui::AcoustidSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("acoustidsettingspage"));}
 void appearancesettingspage(){QWidget form;Ui::AppearanceSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("appearancesettingspage"));}
 void backendsettingspage(){QWidget form;Ui::BackendSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("backendsettingspage"));}
 void behavioursettingspage(){QWidget form;Ui::BehaviourSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("behavioursettingspage"));}
 void collectionsettingspage(){QWidget form;Ui::CollectionSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("collectionsettingspage"));}
 void contextsettingspage(){QWidget form;Ui::ContextSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("contextsettingspage"));}
 void coverssettingspage(){QWidget form;Ui::CoversSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("coverssettingspage"));}
 void globalshortcutssettingspage(){QWidget form;Ui::GlobalShortcutsSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("globalshortcutssettingspage"));}
 void lyricssettingspage(){QWidget form;Ui::LyricsSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("lyricssettingspage"));}
 void networkproxysettingspage(){QWidget form;Ui::NetworkProxySettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("networkproxysettingspage"));}
 void notificationssettingspage(){QWidget form;Ui::NotificationsSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("notificationssettingspage"));}
 void playlistsettingspage(){QWidget form;Ui::PlaylistSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("playlistsettingspage"));}
 void radiosettingspage(){QWidget form;Ui::RadioSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("radiosettingspage"));}
 void scrobblersettingspage(){QWidget form;Ui::ScrobblerSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("scrobblersettingspage"));}
 void settingsdialog(){QDialog form;Ui::SettingsDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("settingsdialog"));}
 void transcodersettingspage(){QWidget form;Ui::TranscoderSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("transcodersettingspage"));}
 void waveformsettingspage(){QWidget form;Ui::WaveformSettingsPage ui;ui.setupUi(&form);capture(form,QStringLiteral("waveformsettingspage"));}
 void aboutdialog(){QDialog form;Ui::AboutDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("aboutdialog"));}
 void addstreamdialog(){QDialog form;Ui::AddStreamDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("addstreamdialog"));}
 void console(){QDialog form;Ui::Console ui;ui.setupUi(&form);capture(form,QStringLiteral("console"));}
 void edittagdialog(){QDialog form;Ui::EditTagDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("edittagdialog"));}
 void errordialog(){QDialog form;Ui::ErrorDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("errordialog"));}
 void messagedialog(){QDialog form;Ui::MessageDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("messagedialog"));}
 void saveplaylistsdialog(){QDialog form;Ui::SavePlaylistsDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("saveplaylistsdialog"));}
 void trackselectiondialog(){QDialog form;Ui::TrackSelectionDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("trackselectiondialog"));}
 void userpassdialog(){QDialog form;Ui::UserPassDialog ui;ui.setupUi(&form);capture(form,QStringLiteral("userpassdialog"));}
};
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName(QStringLiteral("NASAQ-SettingsReview"));app.setApplicationName(QStringLiteral("Isolated"));QTemporaryDir temp;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());Q_INIT_RESOURCE(data);qApp->setProperty("nasaqLiveLanguage",QStringLiteral("ar"));ApplyNasaqTheme();SettingsFormsReview test;return QTest::qExec(&test,argc,argv);}
#include "settingsforms_test.moc"

