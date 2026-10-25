// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QWidget>
#include <QPainter>
// Paint at the window's native device resolution, including monitor/DPI changes.
class NasaqBrandMark final : public QWidget {
 public:
 explicit NasaqBrandMark(QWidget *parent=nullptr):QWidget(parent){setFixedSize(56,56);setAccessibleName(QStringLiteral("NASAQ / نسق"));}
 protected:
 void paintEvent(QPaintEvent *) override {
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
  const qreal side=qMin(width(),height());p.translate((width()-side)/2,(height()-side)/2);p.scale(side/256.0,side/256.0);
  p.setPen(Qt::NoPen);p.setBrush(QColor("#193f34"));p.drawRoundedRect(QRectF(0,0,256,256),64,64);
  p.setPen(Qt::NoPen);p.setBrush(QColor("#dde3ce"));
  p.drawRoundedRect(QRectF(52,70,40,132),13,13);p.drawRoundedRect(QRectF(108,54,40,54),13,13);p.drawRoundedRect(QRectF(164,70,40,132),13,13);
 }
};
