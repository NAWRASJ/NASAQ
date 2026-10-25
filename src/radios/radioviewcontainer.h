/*
 * Strawberry Music Player
 * Copyright 2021, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef RADIOVIEWCONTAINER_H
#define RADIOVIEWCONTAINER_H

#include <QWidget>
#include <QJsonArray>

#include "ui_radioviewcontainer.h"

class RadioView;
class RadioBrowserSearchView;
class QLineEdit;
class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;
class QCheckBox;
class QMimeData;
class QGridLayout;

class RadioViewContainer : public QWidget {
  Q_OBJECT

 public:
  explicit RadioViewContainer(QWidget *parent = nullptr);
  ~RadioViewContainer() override;

  void ReloadSettings();

  RadioView *view() const { return ui_->view; }
  RadioBrowserSearchView *search_view() const { return ui_->search_view; }

 Q_SIGNALS:
  void Refresh();
  void AddToPlaylist(QMimeData *data);

 protected:
  void changeEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

 private:
  void SetupDiscovery();
  void TranslateDiscovery();
  void FilterStations();
  void SelectStation();
  void OpenStation(bool play);
  QJsonObject SelectedStation() const;
  QWidget *discovery_ = nullptr;
  QLineEdit *search_ = nullptr;
  QComboBox *genre_ = nullptr;
  QComboBox *venue_ = nullptr;
  QCheckBox *favorites_only_ = nullptr;
  QListWidget *stations_ = nullptr;
  QLabel *description_ = nullptr;
  QLabel *note_ = nullptr;
  QPushButton *play_ = nullptr;
  QPushButton *add_ = nullptr;
  QPushButton *favorite_ = nullptr;
  QPushButton *homepage_ = nullptr;
  QGridLayout *actions_ = nullptr;
  int action_columns_ = 0;
  QJsonArray catalog_;
  QStringList favorites_;
  Ui_RadioViewContainer *ui_;
};

#endif  // RADIOVIEWCONTAINER_H
