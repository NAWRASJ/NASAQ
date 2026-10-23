// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QMainWindow>
#include <QPointer>
class QDialog;
#include <QLabel>
#include <QTableWidget>
#include <QListWidget>
#include <QStackedWidget>
#include "audio.h"
#include "licenseactivation.h"
#include "scheduler.h"
#include "prayers.h"
#include "studio.h"
class Window:public QMainWindow {
 Q_OBJECT
 public: explicit Window(Store&s,MusicTransport*transport=nullptr,QWidget*notificationHost=nullptr);
 ~Window() override;
 static QStringList sectionNames();
 void openSection(int index);
 void activateLicense(const QString&key){activation.activate(key);}
 bool licenseAllowed()const{return activation.allowed();}
 bool operatorLocked()const{return locked;}
 Q_SIGNALS: void operatorLockChanged(bool locked);
 void licenseAllowedChanged(bool allowed);
 void interfaceLanguageChanged();
 protected: bool event(QEvent *event)override; void closeEvent(QCloseEvent*event)override;
 private:
 QPointer<QDialog> prayerNotice;
 QListWidget *navigation=nullptr;QStackedWidget *sections=nullptr;
 void guarded(const std::function<void()>&fn);
 void showActivationDialog();void announcements();void prayerSettings();
 bool activationDialogOpen=false,lastLicenseAllowed=false;
 bool unlock(); void refresh(); void addEvent(); void importPrayers(); void license();void pdf();void updates();
 Store &store;PrayerService prayers;Scheduler scheduler;Audio audio;Studio studio;StudioEngine studioEngine;Activation activation; QLabel*status;QTableWidget*events;bool locked=false;
};
