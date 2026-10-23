// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QToolButton>
#include <QUndoStack>
#include "fileview/fileview.h"
class FileViewTest : public QObject {
  Q_OBJECT
 private Q_SLOTS:
  void savedFolderDoesNotEmitHomeFirst() {
    QTemporaryDir dir; QVERIFY(dir.isValid()); FileView view;
    QSignalSpy paths(&view, &FileView::PathChanged); view.SetPath(dir.path()); view.show();
    QTRY_COMPARE(paths.size(), 1); QCOMPARE(paths.first().first().toString(), dir.path());
    QCOMPARE(view.findChild<QFileSystemModel*>()->rootPath(), dir.path());
  }
  void driveOverviewAndBack() {
    FileView view; view.SetPath(QStringLiteral("C:/")); view.show();
    auto model=view.findChild<QFileSystemModel*>(); QVERIFY(model);
    auto up=view.findChild<QToolButton*>(QStringLiteral("up")); QVERIFY(up->isEnabled()); up->click();
    QCOMPARE(model->rootPath(), QStringLiteral("")); QVERIFY(!up->isEnabled());
    QTRY_VERIFY(model->rowCount()>0);
    view.findChild<QUndoStack*>()->undo(); QCOMPARE(model->rootPath(), QStringLiteral("C:/"));
    view.findChild<QUndoStack*>()->redo(); QCOMPARE(model->rootPath(), QStringLiteral(""));
  }
  void emptySavedLocationRestoresDrives() {
    FileView view; view.SetPath(QStringLiteral("")); QSignalSpy paths(&view,&FileView::PathChanged); view.show();
    QTRY_COMPARE(paths.size(),1); QVERIFY(paths.first().first().toString().isEmpty());
    QVERIFY(view.findChild<QFileSystemModel*>()->rootPath().isEmpty());
  }
  void typingWaitsForEnter() {
    QTemporaryDir dir; QVERIFY(dir.isValid()); FileView view; view.SetPath(QStringLiteral("C:/")); view.show();
    auto edit=view.findChild<QLineEdit*>(QStringLiteral("path")); auto model=view.findChild<QFileSystemModel*>();
    edit->setText(dir.path()); QCOMPARE(model->rootPath(),QStringLiteral("C:/"));
    QTest::keyClick(edit,Qt::Key_Return); QCOMPARE(model->rootPath(),dir.path());
    edit->setText(dir.filePath(QStringLiteral("missing"))); QTest::keyClick(edit,Qt::Key_Return); QCOMPARE(model->rootPath(),dir.path());
  }
  void removedFolderFallsBack() {
    QTemporaryDir dir; FileView view; view.SetPath(dir.filePath(QStringLiteral("missing"))); view.show();
    QVERIFY(QFileInfo(view.findChild<QFileSystemModel*>()->rootPath()).isDir());
  }
};
int main(int argc,char **argv) {
  QApplication app(argc,argv); app.setOrganizationName(QStringLiteral("NASAQ-Test"));
  app.setApplicationName(QStringLiteral("FileView-")+QUuid::createUuid().toString());
  FileViewTest test; return QTest::qExec(&test,argc,argv);
}
#include "fileview_test.moc"
