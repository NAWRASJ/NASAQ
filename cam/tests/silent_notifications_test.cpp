// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QApplication>
#include <QTimer>
#include <QPushButton>
#include "silentmessagebox.h"
class SilentNotificationsTest : public QObject {
 Q_OBJECT
 private Q_SLOTS:
  void constructorsPreserveVisualAlerts() {
    SilentMessageBox plain;
    QVERIFY(plain.testOption(QMessageBox::Option::DontUseNativeDialog));
    for(auto icon : {QMessageBox::Information,QMessageBox::Warning,QMessageBox::Critical,QMessageBox::Question}) {
      SilentMessageBox box(icon,QStringLiteral("NASAQ"),QStringLiteral("Visible alert"),QMessageBox::Yes|QMessageBox::No);
      QVERIFY(box.testOption(QMessageBox::Option::DontUseNativeDialog));
      QCOMPARE(box.icon(),icon); QCOMPARE(box.text(),QStringLiteral("Visible alert"));
      QCOMPARE(box.standardButtons(),QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::No));
    }
  }
  void staticAlertsRemainSilentAndReturnChosenButton() {
    for(int kind=0;kind<4;++kind) {
      bool checked=false;
      QTimer::singleShot(0,[&checked]{
        auto box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if(!box) return;
        checked=box->testOption(QMessageBox::Option::DontUseNativeDialog)&&box->defaultButton()==box->button(QMessageBox::No);
        box->button(QMessageBox::No)->click();
      });
      auto result=QMessageBox::NoButton;
      const auto title=QStringLiteral("NASAQ"), text=QStringLiteral("Keep existing settings?");
      const auto buttons=QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::No);
      if(kind==0) result=SilentMessageBox::information(nullptr,title,text,buttons,QMessageBox::No);
      if(kind==1) result=SilentMessageBox::warning(nullptr,title,text,buttons,QMessageBox::No);
      if(kind==2) result=SilentMessageBox::critical(nullptr,title,text,buttons,QMessageBox::No);
      if(kind==3) result=SilentMessageBox::question(nullptr,title,text,buttons,QMessageBox::No);
      QVERIFY(checked); QCOMPARE(result,QMessageBox::No);
    }
  }
};
QTEST_MAIN(SilentNotificationsTest)
#include "silent_notifications_test.moc"
