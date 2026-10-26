// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QWidget>
#include <QUrl>
#include <QImage>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <memory>
#include "store.h"
class QScreen;
class QMainWindow;
class QDockWidget;
class QSlider;
class QLabel;
class QToolButton;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

// External audience surfaces remain passive; only the local preview enables controls.
class AudienceSurface final : public QWidget {
 Q_OBJECT
public:
 explicit AudienceSurface(QWidget *parent=nullptr);
 void present(const QImage &frame,bool prayer,bool blocked,bool crop,const QImage &waiting,const QImage &prayerImage);
 bool prayerVisible() const {return prayer_;}
 void setPlayback(bool playing,bool radio,bool reconnecting,const QString &title,const QString &countdown,const QVector<float> &spectrum);
 double rotation() const {return angle_;}
 double tonearmAngle() const {return prayer_||blocked_?-48.:-22.+20.*(radio_?.5:durationSeconds_>0?qBound(0.,double(positionSeconds_)/durationSeconds_,1.):0.);}
 void setAlbumCover(const QImage &cover){cover_=cover;update();}
 bool hasAlbumCover() const {return !cover_.isNull();}
 void enableVideoControls();
 void setTimeline(qint64 positionSeconds,qint64 durationSeconds);
 void revealControls();
 void setVolume(int volume);
 void setNavigation(bool previous,bool next);
Q_SIGNALS:
 void playPauseRequested();
 void stopRequested();
 void previousRequested();
 void nextRequested();
 void seekRequested(quint64 seconds);
 void fullscreenRequested();
 void volumeRequested(int volume);
protected:
 bool eventFilter(QObject *,QEvent *) override;
 void resizeEvent(QResizeEvent *) override;
 void mouseMoveEvent(QMouseEvent *) override;
 void paintEvent(QPaintEvent*) override;
 void contextMenuEvent(QContextMenuEvent*) override;
 void keyPressEvent(QKeyEvent*) override;
private:
 QImage frame_,waiting_,prayerImage_,cover_;
 bool prayer_=false,blocked_=false,crop_=false;
 bool playing_=false,radio_=false,reconnecting_=false;
 QString title_,countdown_;
 QVector<float> spectrum_;
 QElapsedTimer clock_;
 double angle_=0,phase_=0;
 QWidget *controls_=nullptr;
 QSlider *seek_=nullptr,*volume_=nullptr;
 QGraphicsOpacityEffect *opacity_=nullptr;
 QPropertyAnimation *fade_=nullptr;
 QLabel *time_=nullptr;
 QToolButton *playButton_=nullptr;
 QTimer hideControls_;
 bool keyboardControls_=false;
 qint64 durationSeconds_=0,positionSeconds_=0;
 bool controlsAvailable() const {return !prayer_&&!blocked_;}
 bool videoVisible() const {return !frame_.isNull()&&!blocked_&&!prayer_;}
 void syncControls(bool wasVideo);
};

class AudienceDisplay final : public QObject {
 Q_OBJECT
public:
 AudienceDisplay(Store &store,QMainWindow *main);
 ~AudienceDisplay() override;
 void settings();
 void updateLanguage();
 void updateFrame(const QImage &frame,int generation,bool prayer,bool blocked);
 void setPlayback(bool playing,bool radio,bool reconnecting,const QString &title,const QString &countdown,const QVector<float> &spectrum);
 void showFloating();
 void manualMediaRequested(const QUrl &url);
 void setTimeline(qint64 positionSeconds,qint64 durationSeconds);
 void beginTrack(const QString &key);
 void setAlbumCover(const QString &key,const QImage &cover,bool genuine);
 static QString prayerCaption(Store &store,const QDateTime &now);
 bool externalActive() const;
 bool externalEnabled() const {return enabled_;}
 void stopExternal();
 static QString screenKey(QScreen *screen);
 static QString importImage(const QString &source,const QString &directory);
 AudienceSurface *preview() const {return preview_;}
Q_SIGNALS:
 void safetyStopRequested();
 void audioSettingsRequested();
 void playPauseRequested();
 void stopRequested();
 void previousRequested();
 void nextRequested();
 void seekRequested(quint64 seconds);
 void volumeRequested(int volume);
private:
 bool eventFilter(QObject *object,QEvent *event) override;
 void reconcileScreens();
 void repaintSurfaces();
 void reloadImages();
 Store &store_;
 QMainWindow *main_;
 std::unique_ptr<QMainWindow> floating_;
 AudienceSurface *preview_;
 std::unique_ptr<AudienceSurface> audience_;
 QPointer<QScreen> target_;
 QImage frame_,waiting_,prayerImage_;
 QString chosen_;
 QString mediaKey_;
 int generation_=-1;
 bool enabled_=false,prayer_=false,blocked_=true,crop_=false;
 bool wasPlaying_=false;
 bool userHidden_=false;
};

