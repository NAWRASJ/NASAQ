// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QMessageBox>

// Visual alerts must never invoke Windows' audible native message dialogs.
// Keep icons, accessible content, standard buttons and default/escape behavior.
class SilentMessageBox : public QMessageBox {
 public:
  explicit SilentMessageBox(QWidget *parent = nullptr) : QMessageBox(parent) {
    setOption(QMessageBox::Option::DontUseNativeDialog);
  }
  SilentMessageBox(Icon icon, const QString &title, const QString &text,
                   StandardButtons buttons = NoButton, QWidget *parent = nullptr,
                   Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint)
      : QMessageBox(parent) {
    setOption(QMessageBox::Option::DontUseNativeDialog);
    setWindowFlags(flags); setIcon(icon); setWindowTitle(title); setText(text);
    setStandardButtons(buttons);
  }
  static StandardButton information(QWidget *p, const QString &title, const QString &text,
      StandardButtons buttons = Ok, StandardButton selected = NoButton) {
    return Show(p, Information, title, text, buttons, selected);
  }
  static StandardButton warning(QWidget *p, const QString &title, const QString &text,
      StandardButtons buttons = Ok, StandardButton selected = NoButton) {
    return Show(p, Warning, title, text, buttons, selected);
  }
  static StandardButton critical(QWidget *p, const QString &title, const QString &text,
      StandardButtons buttons = Ok, StandardButton selected = NoButton) {
    return Show(p, Critical, title, text, buttons, selected);
  }
  static StandardButton question(QWidget *p, const QString &title, const QString &text,
      StandardButtons buttons = StandardButtons(Yes | No), StandardButton selected = NoButton) {
    return Show(p, Question, title, text, buttons, selected);
  }
 private:
  static StandardButton Show(QWidget *p, Icon icon, const QString &title, const QString &text,
      StandardButtons buttons, StandardButton selected) {
    SilentMessageBox box(icon, title, text, buttons, p);
    if (selected != NoButton) box.setDefaultButton(selected);
    const int result = box.exec();
    return result == -1 ? Cancel : static_cast<StandardButton>(result);
  }
};
