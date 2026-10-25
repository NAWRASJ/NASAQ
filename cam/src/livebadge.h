// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <cmath>
class NasaqLiveBadge final : public QWidget {
 public:
 explicit NasaqLiveBadge(QWidget *parent=nullptr):QWidget(parent){setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);timer_.setInterval(40);connect(&timer_,&QTimer::timeout,this,[this]{update();});elapsed_.start();hide();}
 void setLive(bool live,bool playing,bool arabic){arabic_=arabic;pulse_=live&&playing;setVisible(live);setAccessibleName(arabic?QStringLiteral("بث مباشر"):QStringLiteral("Live radio"));updateGeometry();if(pulse_&&isVisible())timer_.start();else timer_.stop();update();}
 QSize sizeHint()const override{QFont f=font();f.setWeight(QFont::DemiBold);return QSize(QFontMetrics(f).horizontalAdvance(label())+36,28);}
 protected:
 void hideEvent(QHideEvent *e) override {timer_.stop();QWidget::hideEvent(e);}
 void showEvent(QShowEvent *e) override {QWidget::showEvent(e);if(pulse_)timer_.start();}
 void paintEvent(QPaintEvent*)override{
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
  const qreal strength=pulse_?(0.5+0.5*std::sin(elapsed_.elapsed()*6.283185307/1800.0)):0.0;
  const QPointF center(arabic_?width()-14:14,height()/2.0);
  if(pulse_){p.setPen(Qt::NoPen);p.setBrush(QColor(185,48,55,int(12+28*strength)));p.drawEllipse(center,11+strength*2,11+strength*2);}
  p.setPen(QPen(QColor("#b93037"),1.5,Qt::SolidLine,Qt::RoundCap));p.setBrush(Qt::NoBrush);
  for(int radius:{6,10}){QRectF ring(center.x()-radius,center.y()-radius,2*radius,2*radius);p.drawArc(ring,-45*16,90*16);p.drawArc(ring,135*16,90*16);}
  p.setPen(Qt::NoPen);p.setBrush(QColor("#b93037"));p.drawEllipse(center,2.5,2.5);
  p.setPen(QColor("#b93037"));QFont f=font();f.setWeight(QFont::DemiBold);p.setFont(f);
  QRect text=rect().adjusted(arabic_?0:32,0,arabic_?-32:0,0);p.drawText(text,(arabic_?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,label());
 }
 private:
 QString label()const{return arabic_?QStringLiteral("بث مباشر"):QStringLiteral("Live radio");}
 QTimer timer_;QElapsedTimer elapsed_;bool pulse_=false,arabic_=false;
};
