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
 // The deck mount remains fixed. Only the bearing, counterweight and arm rotate.
 const QPointF pivot(1056,247),assetPivot(1095,270);
 p.save();p.translate(pivot);p.scale(.60,.60);p.translate(-assetPivot);
 QPainterPath mount;mount.addEllipse(QRectF(965,180,270,240));mount.addRect(QRectF(1200,290,100,180));p.setClipPath(mount);p.drawImage(QPointF(0,0),arm);p.restore();
 p.save();p.translate(pivot);p.rotate(armAngle);p.scale(.60,.60);p.translate(-assetPivot);
 QPainterPath moving;moving.addRect(QRectF(1030,0,140,260));moving.addEllipse(QRectF(1025,200,140,140));
 QPolygonF tube;tube<<QPointF(1050,260)<<QPointF(1130,280)<<QPointF(1035,500)<<QPointF(810,770)<<QPointF(550,940)<<QPointF(500,1060)<<QPointF(275,1060)<<QPointF(275,830)<<QPointF(780,570)<<QPointF(1000,350);moving.addPolygon(tube);moving.setFillRule(Qt::WindingFill);
 p.setClipPath(moving);p.drawImage(QPointF(0,0),arm);p.restore();
 p.restore();
}
