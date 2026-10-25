/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2018, Vikram Ambrose <ambroseworks@gmail.com>
 * Copyright 2018-2024, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <algorithm>

#include <QTabBar>
#include <QString>
#include <QFontMetrics>
#include <QFont>
#include <QStylePainter>
#include <QPaintEvent>
#include <QTransform>
#include <QLinearGradient>
#include <QColor>
#include <QPen>

#include "core/stylehelper.h"
#include "fancytabbar.h"
#include "fancytabwidget.h"
#include "fancytabdata.h"

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr int TabSize_LargeSidebarMinWidth = 70;
}  // namespace

FancyTabBar::FancyTabBar(QWidget *parent)
    : QTabBar(parent),
      mouseHoverTabIndex(-1) {

  setMouseTracking(true);

}

QString FancyTabBar::TabText(const int index) const {
  return tabText(index).isEmpty() || !tabData(index).isValid() ? ""_L1 : tabData(index).value<FancyTabData*>()->label();
}

QSize FancyTabBar::sizeHint() const {

  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (!tabWidget || tabWidget->mode() == FancyTabWidget::Mode::Tabs || tabWidget->mode() == FancyTabWidget::Mode::IconOnlyTabs) {
    return QTabBar::sizeHint();
  }

  QSize size;
  int h = 0;
  for (int i = 0; i < count(); ++i) {
    if (tabSizeHint(i).width() > size.width()) size.setWidth(tabSizeHint(i).width());
    h += tabSizeHint(i).height();
  }
  size.setHeight(h);

  return size;

}

int FancyTabBar::width() const {

  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (tabWidget && (tabWidget->mode() == FancyTabWidget::Mode::LargeSidebar || tabWidget->mode() == FancyTabWidget::Mode::SmallSidebar)) {
    int w = 0;
    for (int i = 0; i < count(); ++i) {
      if (tabSizeHint(i).width() > w) w = tabSizeHint(i).width();
    }
    return w;
  }
  else {
    return QTabBar::width();
  }

}

QSize FancyTabBar::tabSizeHint(const int index) const {
  auto *tabs = qobject_cast<FancyTabWidget*>(parentWidget());
  const int sidebarHeight=tabs?qMax(54,(tabs->height()-4)/qMax(1,count())):54;
  if (tabs && tabs->mode() == FancyTabWidget::Mode::IconsSidebar) return QSize(58,sidebarHeight);
  // Avoid asking the stylesheet-backed base style for a size: it can re-enter
  // this custom tab bar's sizeHint while the application's style is changing.
  if (tabs && (tabs->mode() == FancyTabWidget::Mode::Tabs || tabs->mode() == FancyTabWidget::Mode::IconOnlyTabs)) return QSize(150,44);
  Q_UNUSED(index);
  return QSize(190,sidebarHeight);
}

void FancyTabBar::leaveEvent(QEvent *event) {

  Q_UNUSED(event);

  mouseHoverTabIndex = -1;
  update();

}

void FancyTabBar::mouseMoveEvent(QMouseEvent *event) {

  QPoint pos = event->pos();

  mouseHoverTabIndex = tabAt(pos);
  if (mouseHoverTabIndex > -1) {
    update();
  }
  QTabBar::mouseMoveEvent(event);

}

void FancyTabBar::paintEvent(QPaintEvent *pe) {
  auto *tabs = qobject_cast<FancyTabWidget*>(parentWidget());
  if (!tabs || tabs->mode() == FancyTabWidget::Mode::Tabs || tabs->mode() == FancyTabWidget::Mode::IconOnlyTabs) { QTabBar::paintEvent(pe); return; }
  QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(rect(), QColor(239, 238, 231));
  for (int i = 0; i < count(); ++i) {
    const QRect r = tabRect(i).adjusted(6, 4, -6, -4);
    const bool selected = i == currentIndex();
    p.setPen(Qt::NoPen); p.setBrush(selected ? QColor(21, 62, 52) : i == mouseHoverTabIndex ? QColor(226, 231, 216) : QColor(239, 238, 231));
    p.drawRoundedRect(r, 8, 8);
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    if (selected) { p.setBrush(QColor(184, 199, 154)); p.drawRoundedRect(QRect(rtl ? r.right()-2 : r.left(), r.top()+10, 3, r.height()-20), 1, 1); }
    const bool iconsOnly=tabs->mode()==FancyTabWidget::Mode::IconsSidebar;
    const int iconSize=22,gap=10;
    const QString text=fontMetrics().elidedText(TabText(i),rtl?Qt::ElideLeft:Qt::ElideRight,qMax(0,r.width()-iconSize-gap-24));
    const int textWidth=iconsOnly?0:fontMetrics().horizontalAdvance(text);
    const int groupWidth=iconSize+(iconsOnly?0:gap+textWidth);
    const int groupLeft=r.left()+(r.width()-groupWidth)/2;
    const int iconLeft=iconsOnly?groupLeft:rtl?groupLeft+textWidth+gap:groupLeft;
    tabIcon(i).paint(&p,QRect(iconLeft,r.center().y()-iconSize/2,iconSize,iconSize),Qt::AlignCenter,!isTabEnabled(i)?QIcon::Disabled:selected?QIcon::Selected:QIcon::Normal);
    p.setPen(isTabEnabled(i)?(selected?QColor(255,254,250):QColor(21,62,52)):QColor(120,130,120));
    if(!iconsOnly)p.drawText(QRect(rtl?groupLeft:groupLeft+iconSize+gap,r.top(),textWidth,r.height()),Qt::AlignCenter,text);

  }
}
