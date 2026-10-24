// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QWidget>
#include <functional>
class QLabel;
class QPushButton;
struct StoreUpdateResult { bool available=false; QString error; };
StoreUpdateResult queryStoreUpdates(quintptr owner);
class StoreUpdates : public QWidget {
 Q_OBJECT
public:
 using Checker=std::function<StoreUpdateResult(quintptr)>;
 explicit StoreUpdates(bool packaged,QWidget *parent=nullptr,Checker checker=queryStoreUpdates);
 void check();
 void updateLanguage();
 static bool openStore();
Q_SIGNALS:
 void availabilityChanged(bool available);
private:
 bool packaged,busy=false,checked=false,available=false;
 QString error;
 Checker checker;
 QLabel *status;
 QPushButton *checkButton,*openButton;
};
