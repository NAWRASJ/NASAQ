// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "nasaqlanguage.h"
#include <QtWidgets>
// Translate only UI chrome. Never traverse playlist cells, editable inputs or user content.
inline QString NasaqChromeText(const QString &text){
 for(auto it=NasaqCatalog().cbegin();it!=NasaqCatalog().cend();++it)
  if(text==it.key()||text==it.value().first||text==it.value().second)return NText(it.key());
 return text;
}
inline void ApplyNasaqLanguage(const QString &choice){
 QSettings(QStringLiteral("NASAQ"),QStringLiteral("Preferences")).setValue("language",choice);
 const auto locale=choice=="system"?QLocale::system().name():choice;
 qApp->setProperty("nasaqLiveLanguage",locale.startsWith("ar")?"ar":"en");
 const auto direction=NasaqLanguage()=="ar"?Qt::RightToLeft:Qt::LeftToRight;
 qApp->setLayoutDirection(direction);
 qApp->setApplicationDisplayName(NText("NASAQ")+" " NASAQ_VERSION_STRING);
 for(auto widget:QApplication::allWidgets()){
  if(widget->isWindow()||widget->layout())widget->setLayoutDirection(direction);
  if(auto b=qobject_cast<QAbstractButton*>(widget))b->setText(NasaqChromeText(b->text()));
  if(auto l=qobject_cast<QLabel*>(widget))l->setText(NasaqChromeText(l->text()));
  if(auto g=qobject_cast<QGroupBox*>(widget))g->setTitle(NasaqChromeText(g->title()));
  if(auto c=qobject_cast<QComboBox*>(widget);c&&!c->isEditable()){QSignalBlocker block(c);for(int i=0;i<c->count();++i)c->setItemText(i,NasaqChromeText(c->itemText(i)));}
  if(auto list=qobject_cast<QListWidget*>(widget);list&&list->objectName()=="sectionNavigation")for(int i=0;i<list->count();++i)list->item(i)->setText(NasaqChromeText(list->item(i)->text()));
  if(auto tabs=qobject_cast<QTabBar*>(widget);tabs&&qobject_cast<QTabWidget*>(tabs->parentWidget()))for(int i=0;i<tabs->count();++i){tabs->setTabText(i,NasaqChromeText(tabs->tabText(i)));tabs->setTabToolTip(i,NasaqChromeText(tabs->tabToolTip(i)));}
  if(auto table=qobject_cast<QTableWidget*>(widget))for(int i=0;i<table->columnCount();++i)if(auto h=table->horizontalHeaderItem(i))h->setText(NasaqChromeText(h->text()));
  for(auto a:widget->actions())a->setText(NasaqChromeText(a->text()));
  widget->setWindowTitle(NasaqChromeText(widget->windowTitle()));
 }
}
