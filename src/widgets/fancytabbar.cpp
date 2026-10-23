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
  if (tabs && tabs->mode() == FancyTabWidget::Mode::IconsSidebar) return QSize(58,54);
  // Avoid asking the stylesheet-backed base style for a size: it can re-enter
  // this custom tab bar's sizeHint while the application's style is changing.
  if (tabs && (tabs->mode() == FancyTabWidget::Mode::Tabs || tabs->mode() == FancyTabWidget::Mode::IconOnlyTabs)) return QSize(150,44);
  Q_UNUSED(index);
  return QSize(190,54);
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
  p.fillRect(rect(), QColor(12, 17, 20));
  for (int i = 0; i < count(); ++i) {
    const QRect r = tabRect(i).adjusted(6, 4, -6, -4);
    const bool selected = i == currentIndex();
    p.setPen(Qt::NoPen); p.setBrush(selected ? QColor(24, 58, 55) : i == mouseHoverTabIndex ? QColor(27, 35, 39) : QColor(12, 17, 20));
    p.drawRoundedRect(r, 8, 8);
    if (selected) { p.setBrush(QColor(41, 211, 187)); p.drawRoundedRect(QRect(r.left(), r.top()+10, 3, r.height()-20), 1, 1); }
    tabIcon(i).paint(&p, QRect(r.left()+14, r.center().y()-11, 22, 22));
    p.setPen(isTabEnabled(i) ? QColor(221, 232, 230) : QColor(108, 122, 125));
    if (tabs->mode() != FancyTabWidget::Mode::IconsSidebar)
      p.drawText(r.adjusted(46, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, fontMetrics().elidedText(TabText(i), Qt::ElideRight, r.width()-52));
  }
}
