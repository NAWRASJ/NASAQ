// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QPainter>
#include <QImage>
#include <QPainterPath>

// Production photographic layers, bundled in the executable. No network fetch at runtime.
inline void PaintNasaqTurntable(QPainter &p,const QRectF &area,double angle,double armAngle){
 static const QImage base(QStringLiteral(":/nasaq/turntable-base.png"));
 static const QImage record(QStringLiteral(":/nasaq/turntable-record.png"));
 static const QImage arm(QStringLiteral(":/nasaq/turntable-arm.png"));
 p.save();p.translate(area.topLeft());p.scale(area.width()/1254.,area.height()/1254.);
 p.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
 p.drawImage(QRectF(0,0,1254,1254),base);
 const QPointF center(557,541);
 p.save();p.translate(center);p.scale(1,.985);p.rotate(angle);
 p.drawImage(QRectF(-423,-423,846,846),record);
 // Sharp approved NASAQ mark over the textured blank record label.
 p.setPen(Qt::NoPen);p.setBrush(QColor("#173f34"));
 p.drawRoundedRect(QRectF(-44,-76,19,60),6,6);p.drawRoundedRect(QRectF(-10,-86,19,28),6,6);p.drawRoundedRect(QRectF(24,-76,19,60),6,6);
 QFont f=p.font();f.setPixelSize(19);f.setBold(true);f.setLetterSpacing(QFont::AbsoluteSpacing,5);p.setFont(f);p.setPen(QColor("#173f34"));
 p.drawText(QRectF(-110,37,220,30),Qt::AlignCenter,QStringLiteral("NASAQ"));
 f.setPixelSize(27);f.setLetterSpacing(QFont::AbsoluteSpacing,0);p.setFont(f);p.drawText(QRectF(-100,70,200,38),Qt::AlignCenter,QString::fromUtf8("نسق"));p.restore();
 // Stationary machined spindle above the revolving label.
 QRadialGradient spindle(center-QPointF(4,5),15);spindle.setColorAt(0,QColor("#fffbed"));spindle.setColorAt(.5,QColor("#bfb6a4"));spindle.setColorAt(1,QColor("#655f51"));p.setBrush(spindle);p.setPen(Qt::NoPen);p.drawEllipse(center,13,13);
 // The base image already contains the fixed mounting plate. Drawing the
 // arm asset's second plate here duplicates its rim and screws. Rotate only
 // the counterweight, bearing and tube around the same center as that plate.
 const QPointF pivot(1056,247),assetPivot(1095,250);
 p.save();p.translate(pivot);p.rotate(armAngle);p.scale(.60,.60);p.translate(-assetPivot);
 QPainterPath moving;
 // Draw a clean bearing cap rather than clipping the asset's plate into it.
 QConicalGradient metal(assetPivot,35);
 metal.setColorAt(0,QColor("#eee9da"));metal.setColorAt(.18,QColor("#8e887a"));
 metal.setColorAt(.42,QColor("#faf7eb"));metal.setColorAt(.66,QColor("#9d9687"));metal.setColorAt(1,QColor("#eee9da"));
 p.setPen(QPen(QColor("#625e53"),2));p.setBrush(QColor("#817c70"));p.drawEllipse(QRectF(1028,190,136,144));
 p.setBrush(metal);p.setPen(QPen(QColor("#d3cebf"),2));p.drawEllipse(QRectF(1030,186,132,132));
 moving.addRect(QRectF(1030,0,140,177));
 moving.addRect(QRectF(1075,165,45,30));
 QPainterPath tube;
 tube.moveTo(1080,256);tube.lineTo(1118,270);
 tube.cubicTo(1075,455,920,705,555,860);
 tube.lineTo(520,935);tube.lineTo(500,1060);tube.lineTo(275,1060);tube.lineTo(275,830);
 tube.lineTo(500,790);tube.lineTo(535,805);
 tube.cubicTo(855,645,1010,455,1080,256);tube.closeSubpath();
 moving.addPath(tube);moving.setFillRule(Qt::WindingFill);
 p.setClipPath(moving,Qt::IntersectClip);p.drawImage(QPointF(0,0),arm);p.restore();
 p.restore();
}
