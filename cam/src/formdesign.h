// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>

class NasaqFormColumns final:public QWidget {
 public:
  NasaqFormColumns(){columns=new QBoxLayout(QBoxLayout::TopToBottom,this);columns->setContentsMargins(0,0,0,0);columns->setSpacing(16);}
  void addWidget(QWidget *widget){columns->addWidget(widget,1);}
 protected:
  void resizeEvent(QResizeEvent *event)override{
   const auto direction=event->size().width()>=900?QBoxLayout::LeftToRight:QBoxLayout::TopToBottom;
   if(columns->direction()!=direction)columns->setDirection(direction);
   QWidget::resizeEvent(event);
  }
 private:QBoxLayout *columns;
};

// Width expresses the expected input, not the size of the surrounding window.
// Logical pixels and font metrics preserve readability under Windows scaling.
inline void ApplyNasaqFormDesign(QWidget *root) {
 for(auto form:root->findChildren<QFormLayout*>()){
  form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  form->setRowWrapPolicy(QFormLayout::WrapLongRows);
  form->setHorizontalSpacing(20);form->setVerticalSpacing(12);
  form->setFormAlignment(Qt::AlignTop|Qt::AlignLeading);
  for(int row=0;row<form->rowCount();++row){
   auto item=form->itemAt(row,QFormLayout::LabelRole);
   if(item)if(auto label=qobject_cast<QLabel*>(item->widget())){label->setWordWrap(true);label->setFixedWidth(160);}
  }
 }
 for(auto spin:root->findChildren<QAbstractSpinBox*>()){
  spin->ensurePolished();int width=spin->sizeHint().width();
  if(auto number=qobject_cast<QSpinBox*>(spin))width=qMax(width,number->fontMetrics().horizontalAdvance(QString::number(number->maximum())+number->prefix()+number->suffix())+64);
  spin->setFixedWidth(qMax(110,width));spin->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  // Qt reverses date/time sections when direction changes. Preserve the format.
  auto date=qobject_cast<QDateTimeEdit*>(spin);const auto format=date?date->displayFormat():QString();
  spin->setLayoutDirection(Qt::LeftToRight);spin->setAlignment(Qt::AlignLeft);
  if(date)date->setDisplayFormat(format);
 }
 for(auto combo:root->findChildren<QComboBox*>()){
  combo->ensurePolished();int width=180;
  for(int i=0;i<combo->count();++i)width=qMax(width,combo->fontMetrics().horizontalAdvance(combo->itemText(i))+58);
  combo->setMaximumWidth(qMin(520,width));combo->setMinimumWidth(160);
  combo->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
 }
 for(auto edit:root->findChildren<QLineEdit*>()){
  if(qobject_cast<QAbstractSpinBox*>(edit->parentWidget())||qobject_cast<QComboBox*>(edit->parentWidget()))continue;
  edit->setMaximumWidth(edit->echoMode()==QLineEdit::Password?360:560);
 }
 for(auto button:root->findChildren<QPushButton*>())button->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);
}
