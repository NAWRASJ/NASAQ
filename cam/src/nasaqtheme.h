// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QFont>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QEvent>
#include <QRegion>
#include "nasaqlanguage.h"
// Keep native popup corners consistent with the rounded theme on Windows.
class NasaqMenuShape final : public QObject {
 public: using QObject::QObject;
 protected: bool eventFilter(QObject *object,QEvent *event) override {
  if(event->type()==QEvent::Show||event->type()==QEvent::Resize){
   if(auto *menu=qobject_cast<QMenu*>(object)){
    QPainterPath shape;shape.addRoundedRect(QRectF(menu->rect()),10,10);
    menu->setMask(QRegion(shape.toFillPolygon().toPolygon()));
   }
  }
  return false;
 }
};
// Replace platform-era bevelled arrows/close glyphs without changing control behavior.
class NasaqControlStyle final : public QProxyStyle {
 public:
 NasaqControlStyle():QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))){}
 void drawPrimitive(PrimitiveElement element,const QStyleOption *option,QPainter *p,const QWidget *widget=nullptr) const override {
  if(element==PE_IndicatorMenuCheckMark){
    p->save();p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen((option->state&State_Selected)?QColor("#fffefa"):QColor("#153e34"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    const QPointF c=QRectF(option->rect).center();QPainterPath mark;
    mark.moveTo(c+QPointF(-4,0));mark.lineTo(c+QPointF(-1,3));mark.lineTo(c+QPointF(5,-3));p->drawPath(mark);p->restore();return;
  }
  if(element==PE_IndicatorCheckBox||element==PE_IndicatorRadioButton){
    p->save();p->setRenderHint(QPainter::Antialiasing);
    const bool enabled=option->state&State_Enabled;
    const bool checked=option->state&(State_On|State_NoChange);
    QRectF box=QRectF(option->rect).adjusted(1,1,-1,-1);
    p->setPen(QPen(enabled?QColor("#8b9c7b"):QColor("#c7d0bf"),1));
    p->setBrush(checked?(enabled?QColor("#153e34"):QColor("#a5ad9e")):QColor("#fffefa"));
    if(element==PE_IndicatorRadioButton)p->drawEllipse(box);else p->drawRoundedRect(box,4,4);
    if(checked){p->setPen(QPen(QColor("#fffefa"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
      if(element==PE_IndicatorRadioButton){p->setBrush(QColor("#fffefa"));p->setPen(Qt::NoPen);p->drawEllipse(box.center(),2.5,2.5);}
      else if(option->state&State_NoChange)p->drawLine(QPointF(box.left()+3,box.center().y()),QPointF(box.right()-3,box.center().y()));
      else{QPainterPath mark;mark.moveTo(box.left()+3,box.center().y());mark.lineTo(box.center().x()-1,box.bottom()-3);mark.lineTo(box.right()-3,box.top()+3);p->drawPath(mark);}
    }p->restore();return;
  }
  const bool close=element==PE_IndicatorTabClose;
  const bool up=element==PE_IndicatorArrowUp||element==PE_IndicatorSpinUp;
  const bool down=element==PE_IndicatorArrowDown||element==PE_IndicatorSpinDown;
  const bool left=element==PE_IndicatorArrowLeft;
  const bool right=element==PE_IndicatorArrowRight;
  if(!close&&!up&&!down&&!left&&!right){QProxyStyle::drawPrimitive(element,option,p,widget);return;}
  p->save();p->setRenderHint(QPainter::Antialiasing);
  const bool enabled=option->state&State_Enabled;
  const QColor color=enabled?QColor("#153e34"):QColor("#a5ad9e");
  if(close&&(option->state&(State_MouseOver|State_Sunken))){p->setPen(Qt::NoPen);p->setBrush(QColor("#dce5cd"));p->drawRoundedRect(option->rect.adjusted(1,1,-1,-1),5,5);}
  p->translate(QRectF(option->rect).center());p->setPen(QPen(color,1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p->setBrush(Qt::NoBrush);
  const qreal radius=qMin(4.0,qMin(option->rect.width(),option->rect.height())/3.0);
  if(close){p->drawLine(QPointF(-radius,-radius),QPointF(radius,radius));p->drawLine(QPointF(-radius,radius),QPointF(radius,-radius));}
  else {if(up)p->rotate(180);if(left)p->rotate(90);if(right)p->rotate(-90);QPainterPath path;path.moveTo(-radius,-radius/2);path.lineTo(0,radius/2);path.lineTo(radius,-radius/2);p->drawPath(path);}
  p->restore();
 }
};
inline void ApplyNasaqTheme() {
 if(qApp->property("nasaqThemeApplied").toBool())return;
 qApp->setProperty("nasaqThemeApplied",true);
 Q_INIT_RESOURCE(nasaq_brand);
 auto translator=new NasaqTranslator(qApp);qApp->installTranslator(translator);qApp->setLayoutDirection(NasaqLanguage()==QStringLiteral("ar")?Qt::RightToLeft:Qt::LeftToRight);qApp->setApplicationDisplayName(NText(QStringLiteral("NASAQ"))+QStringLiteral(" " NASAQ_VERSION_STRING));
 qApp->setStyle(new NasaqControlStyle);
 qApp->installEventFilter(new NasaqMenuShape(qApp));
 QPalette p;
 p.setColor(QPalette::Window,QColor("#f8f7f2")); p.setColor(QPalette::WindowText,QColor("#153e34"));
 p.setColor(QPalette::Base,QColor("#fffefa")); p.setColor(QPalette::AlternateBase,QColor("#f3f4ed"));
 p.setColor(QPalette::Text,QColor("#153e34")); p.setColor(QPalette::Button,QColor("#eeeee5"));
 p.setColor(QPalette::ButtonText,QColor("#153e34")); p.setColor(QPalette::Highlight,QColor("#dce5cd"));
 p.setColor(QPalette::HighlightedText,QColor("#153e34")); p.setColor(QPalette::Link,QColor("#365d49"));
 p.setColor(QPalette::Disabled,QPalette::Text,QColor("#788278"));
 p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor("#788278"));
 p.setColor(QPalette::Light,QColor("#fffefa")); p.setColor(QPalette::Midlight,QColor("#efeee7"));
 p.setColor(QPalette::Mid,QColor("#d7dcd0")); p.setColor(QPalette::Dark,QColor("#8a9688"));
 p.setColor(QPalette::Shadow,QColor("#637369"));
 qApp->setProperty("nasaqIvoryTheme",true);
 qApp->setPalette(p); qApp->setFont(QFont(QStringLiteral("Segoe UI"),10));
 QFile css(QStringLiteral(":/nasaq/nasaq.qss"));
 if(css.open(QIODevice::ReadOnly))qApp->setStyleSheet(QString::fromUtf8(css.readAll()));
}
