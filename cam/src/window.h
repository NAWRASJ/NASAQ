// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include "audio.h"
#include "scheduler.h"
class Window:public QMainWindow {
 Q_OBJECT
 public: explicit Window(Store&s,MusicTransport*transport=nullptr);
 bool operatorLocked()const{return locked;}
 Q_SIGNALS: void operatorLockChanged(bool locked);
 protected: void closeEvent(QCloseEvent*event)override;
 private:
 void guarded(const std::function<void()>&fn);
 bool unlock(); void refresh(); void addEvent(); void importPrayers(); void license();void pdf();void updates();
 Store &store;Scheduler scheduler;Audio audio; QLabel*status;QTableWidget*events;bool locked=false;
};
