// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QSet>
#include <QIcon>
// Original NASAQ line icons, drawn at the requested resolution (no SVG plugin).
class NasaqLineIconEngine final : public QIconEngine {
 public:
  explicit NasaqLineIconEngine(QByteArray name):name_(std::move(name)){}
  QIconEngine *clone() const override { return new NasaqLineIconEngine(name_); }
  void paint(QPainter *p,const QRect &rect,QIcon::Mode mode,QIcon::State) override {
    p->save(); const qreal side=qMin(rect.width(),rect.height());
    p->translate(rect.x()+(rect.width()-side)/2,rect.y()+(rect.height()-side)/2);p->scale(side/24,side/24);
    p->setRenderHint(QPainter::Antialiasing);
    const QColor color=mode==QIcon::Disabled?QColor(120,130,120):mode==QIcon::Selected?QColor(255,254,250):QColor(21,62,52);
    p->setPen(QPen(color,1.65,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p->setBrush(Qt::NoBrush);
    auto line=[p](qreal x,qreal y,qreal a,qreal b){p->drawLine(QPointF(x,y),QPointF(a,b));};
    auto poly=[p](std::initializer_list<QPointF> points){QPainterPath path;bool first=true;for(auto point:points){if(first){path.moveTo(point);first=false;}else path.lineTo(point);}p->drawPath(path);};
    const auto &n=name_;
    if(n=="media-playback-start") {p->setBrush(color);p->setPen(Qt::NoPen);p->drawPolygon(QPolygonF{QPointF(8,5),QPointF(20,12),QPointF(8,19)});}
    else if(n=="media-playback-pause"){p->setBrush(color);p->setPen(Qt::NoPen);p->drawRoundedRect(QRectF(6,5,4,14),1,1);p->drawRoundedRect(QRectF(14,5,4,14),1,1);}
    else if(n=="media-playback-stop")p->drawRoundedRect(QRectF(5,5,14,14),2,2);
    else if(n=="media-skip-forward"||n=="media-skip-backward") {if(n=="media-skip-backward"){p->translate(24,0);p->scale(-1,1);}poly({{5,5},{16,12},{5,19},{5,5}});line(19,5,19,19);}
    else if(n=="nasaq-open-media"){p->drawRoundedRect(QRectF(3,4,18,16),3,3);poly({{10,8},{16,12},{10,16},{10,8}});}
    else if(n=="nasaq-display"){p->drawRoundedRect(QRectF(2,3,20,14),2,2);line(12,17,12,21);line(7,21,17,21);}
    else if(n=="document-open"||n=="folder"||n=="document-open-folder")poly({{3,19},{3,5},{9,5},{11,8},{21,8},{19,19},{3,19}});
    else if(n=="document-new"){poly({{6,21},{6,3},{15,3},{19,7},{19,21},{6,21}});line(15,3,15,7);line(15,7,19,7);line(9,13,16,13);line(12.5,9.5,12.5,16.5);}
    else if(n=="document-save"){poly({{4,20},{4,4},{17,4},{20,7},{20,20},{4,20}});p->drawRect(QRectF(8,4,8,6));p->drawRoundedRect(QRectF(8,14,8,6),1,1);}
    else if(n=="library-music"){line(4,4,4,20);line(9,4,9,20);poly({{14,5},{17,4},{21,19},{18,20},{14,5}});}
    else if(n=="view-media-playlist"||n=="footsteps"||n=="nasaq-smart-playlist"){line(4,5,19,5);line(4,10,14,10);line(4,15,11,15);if(n=="nasaq-smart-playlist")poly({{18,11},{19.5,15},{23,16.5},{19.5,18},{18,22},{16.5,18},{13,16.5},{16.5,15},{18,11}});else if(n=="footsteps")poly({{15,14},{19,18},{15,22}});else poly({{16,13},{22,17},{16,21},{16,13}});}
    else if(n=="radio"){p->drawRoundedRect(QRectF(3,8,18,13),2,2);line(6,8,18,3);p->drawEllipse(QPointF(8,14.5),2.5,2.5);line(14,12,18,12);line(14,16,18,16);}
    else if(n=="device"){p->drawRoundedRect(QRectF(6,3,12,18),2,2);line(10,17,14,17);}
    else if(n=="edit-find"||n=="document-search"){p->drawEllipse(QPointF(10,10),6,6);line(14.5,14.5,21,21);}
    else if(n=="view-refresh"){p->drawArc(QRectF(4,4,16,16),35*16,290*16);poly({{16,3},{20,6},{21,1.5}});}
    else if(n=="edit-undo"||n=="edit-redo"){if(n=="edit-redo"){p->translate(24,0);p->scale(-1,1);}poly({{8,4},{3,9},{8,14}});QPainterPath path;path.moveTo(3,9);path.lineTo(15,9);path.cubicTo(23,9,23,20,15,20);p->drawPath(path);}
    else if(n=="list-add"||n=="list-remove"){line(5,12,19,12);if(n=="list-add")line(12,5,12,19);}
    else if(n=="edit-clear"||n=="window-close"||n=="application-exit"){line(6,6,18,18);line(18,6,6,18);}
    else if(n=="edit-clear-list"){line(3,5,20,5);line(3,10,14,10);line(3,15,10,15);line(16,14,21,19);line(21,14,16,19);}
    else if(n=="go-home"){poly({{3,11},{12,3},{21,11}});poly({{6,9},{6,21},{10,21},{10,15},{14,15},{14,21},{18,21},{18,9}});}
    else if(n=="go-next"||n=="go-previous"||n=="go-up"||n=="go-down"){p->translate(12,12);if(n=="go-previous")p->rotate(180);if(n=="go-up")p->rotate(-90);if(n=="go-down")p->rotate(90);poly({{-3,-6},{3,0},{-3,6}});}
    else if(n=="applications-internet"||n=="document-open-remote"){p->drawEllipse(QPointF(12,12),9,9);p->drawEllipse(QRectF(8,3,8,18));line(3,12,21,12);}
    else if(n=="love"){QPainterPath path;path.moveTo(12,20);path.cubicTo(0,12,2,3,8,5);path.quadTo(11,5,12,8);path.quadTo(14,3,18,5);path.cubicTo(25,9,18,16,12,20);p->drawPath(path);}
    else if(n=="star"||n=="star-grey"){poly({{12,3},{14.8,8.7},{21,9.6},{16.5,14},{17.6,20.2},{12,17.3},{6.4,20.2},{7.5,14},{3,9.6},{9.2,8.7},{12,3}});}
    else if(n=="configure"){line(4,6,20,6);line(4,12,20,12);line(4,18,20,18);p->setBrush(QColor(248,247,242));p->drawEllipse(QPointF(9,6),2,2);p->drawEllipse(QPointF(16,12),2,2);p->drawEllipse(QPointF(8,18),2,2);}
    p->restore();
  }
  QPixmap pixmap(const QSize &size,QIcon::Mode mode,QIcon::State state) override {QPixmap pm(size);pm.fill(Qt::transparent);QPainter p(&pm);paint(&p,QRect(QPoint(),size),mode,state);return pm;}
 private: QByteArray name_;
};
inline QIcon NasaqLineIcon(const QString &name) {
  static const QSet<QByteArray> names={"media-playback-start","media-playback-pause","media-playback-stop","media-skip-forward","media-skip-backward","document-open","folder","document-open-folder","document-new","document-save","library-music","view-media-playlist","footsteps","nasaq-smart-playlist","radio","device","edit-find","document-search","view-refresh","edit-undo","edit-redo","list-add","list-remove","edit-clear","window-close","application-exit","edit-clear-list","go-home","go-next","go-previous","go-up","go-down","applications-internet","document-open-remote","love","configure","star","star-grey"};
  const auto key=name.toLatin1();return (names.contains(key)||key=="nasaq-open-media"||key=="nasaq-display")?QIcon(new NasaqLineIconEngine(key)):QIcon();
}
