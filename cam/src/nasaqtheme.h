// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QFont>
#include "nasaqlanguage.h"
inline void ApplyNasaqTheme() {
 if(qApp->property("nasaqThemeApplied").toBool())return;
 qApp->setProperty("nasaqThemeApplied",true);
 Q_INIT_RESOURCE(nasaq_brand);
 auto translator=new NasaqTranslator(qApp);qApp->installTranslator(translator);qApp->setLayoutDirection(NasaqLanguage()==QStringLiteral("ar")?Qt::RightToLeft:Qt::LeftToRight);qApp->setApplicationDisplayName(NText(QStringLiteral("NASAQ"))+QStringLiteral(" " NASAQ_VERSION_STRING));
 qApp->setStyle(QStringLiteral("Fusion"));
 QPalette p;
 p.setColor(QPalette::Window,QColor("#0c1114")); p.setColor(QPalette::WindowText,QColor("#e1eae9"));
 p.setColor(QPalette::Base,QColor("#101719")); p.setColor(QPalette::AlternateBase,QColor("#151e21"));
 p.setColor(QPalette::Text,QColor("#e1eae9")); p.setColor(QPalette::Button,QColor("#1b272b"));
 p.setColor(QPalette::ButtonText,QColor("#e1eae9")); p.setColor(QPalette::Highlight,QColor("#29d3bb"));
 p.setColor(QPalette::HighlightedText,QColor("#071512")); p.setColor(QPalette::Link,QColor("#29d3bb"));
 p.setColor(QPalette::Disabled,QPalette::Text,QColor("#7c9094"));
 p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor("#7c9094"));
 qApp->setPalette(p); qApp->setFont(QFont(QStringLiteral("Segoe UI"),10));
 QFile css(QStringLiteral(":/nasaq/nasaq.qss"));
 if(css.open(QIODevice::ReadOnly))qApp->setStyleSheet(QString::fromUtf8(css.readAll()));
}
