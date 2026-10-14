// SPDX-License-Identifier: GPL-3.0-or-later
#include "window.h"
#include <QApplication>
#include <QStandardPaths>
#include <QLockFile>
#include <QMessageBox>
int main(int argc,char**argv){qputenv("QT_MEDIA_BACKEND","windows");QApplication app(argc,argv);app.setApplicationDisplayName("نسق NASAQ");app.setApplicationVersion("0.3.0");app.setOrganizationName("CAMProject");app.setApplicationName("CAM");bool smoke=app.arguments().contains("--smoke-test");QTemporaryDir temporary;auto dir=smoke?temporary.path():QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);QDir().mkpath(dir);QLockFile lock(dir+"/instance.lock");if(!lock.tryLock(100)){QMessageBox::warning(nullptr,"NASAQ","NASAQ is already running or its lock is unavailable.");return 2;}try{Store store(dir+"/cam.sqlite");Window window(store);window.show();if(smoke)QTimer::singleShot(2000,&app,[&]{int i=app.arguments().indexOf("--screenshot");if(i>=0&&i+1<app.arguments().size())window.grab().save(app.arguments()[i+1]);app.quit();});return app.exec();}catch(const std::exception&e){if(smoke){qCritical("%s",e.what());return 1;}QMessageBox::critical(nullptr,"NASAQ startup",e.what());return 1;}}
