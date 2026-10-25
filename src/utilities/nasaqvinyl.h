// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QImage>
#include <QPainter>
#include <QPen>
// Native vector artwork: NASAQ's existing three-band mark, on a forest vinyl.
inline QImage NasaqVinylImage() {
  QImage image(640,640,QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);p.setBrush(QColor(25,63,52));p.drawEllipse(QRectF(24,24,592,592));
  p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor(48,84,68),1.2));
  for(int r=134;r<290;r+=8)p.drawEllipse(QPointF(320,320),r,r);
  p.setPen(QPen(QColor(91,119,88,80),2));
  for(int r=142;r<284;r+=16)p.drawArc(QRectF(320-r,320-r,r*2,r*2),25*16,48*16);
  p.setPen(Qt::NoPen);p.setBrush(QColor(221,227,206));p.drawEllipse(QPointF(320,320),114,114);
  p.setBrush(QColor(25,63,52));
  // Preserve the proportions of branding/nasaq-mark.svg, centered above the spindle.
  p.drawRoundedRect(QRectF(268,265,27,89),8.8,8.8);
  p.drawRoundedRect(QRectF(306,254,27,36),8.8,8.8);
  p.drawRoundedRect(QRectF(344,265,27,89),8.8,8.8);
  p.setBrush(QColor(248,247,242));p.drawEllipse(QPointF(320,320),6,6);
  return image;
}
