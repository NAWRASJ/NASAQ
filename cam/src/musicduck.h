// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QVariantAnimation>
#include <functional>
// Transient gain: never changes the user's saved volume.
class MusicDuck:public QObject {
 public:
 explicit MusicDuck(std::function<void(double)> apply,QObject *parent=nullptr):QObject(parent),applyGain(std::move(apply)){
  animation.setDuration(1000);animation.setEasingCurve(QEasingCurve::InOutSine);
  connect(&animation,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){gain=v.toDouble();applyGain(gain);});
 }
 void press(){animation.stop();gain=.05;applyGain(gain);}
 void release(){animation.stop();animation.setStartValue(gain);animation.setEndValue(1.0);animation.start();}
 void reset(){animation.stop();gain=1.0;applyGain(gain);}
 double value()const{return gain;}
 private:std::function<void(double)> applyGain;QVariantAnimation animation;double gain=1.0;
};
