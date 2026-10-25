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

#include <QWidget>
#include <QSettings>
#include <QToolButton>

#include "core/iconloader.h"
#include "core/settings.h"
#include "constants/appearancesettings.h"
#include "radioviewcontainer.h"
#include "ui_radioviewcontainer.h"

// Curated radio uses the same playlist/player path as all other NASAQ music.
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QEvent>
#include <QSignalBlocker>
#include "radiomimedata.h"
#include <QCoreApplication>
#include <QLocale>


using namespace Qt::Literals::StringLiterals;

RadioViewContainer::RadioViewContainer(QWidget *parent)
    : QWidget(parent),
      ui_(new Ui_RadioViewContainer) {

  ui_->setupUi(this);
  SetupDiscovery();

  QObject::connect(ui_->refresh, &QToolButton::clicked, this, &RadioViewContainer::Refresh);

  ui_->refresh->setIcon(IconLoader::Load(u"view-refresh"_s));

  ReloadSettings();

}

RadioViewContainer::~RadioViewContainer() { delete ui_; }

void RadioViewContainer::ReloadSettings() {

  Settings s;
  s.beginGroup(AppearanceSettings::kSettingsGroup);
  int iconsize = s.value(AppearanceSettings::kIconSizeLeftPanelButtons, AppearanceSettings::kDefaultIconSizeLeftPanelButtons).toInt();
  s.endGroup();

  ui_->refresh->setIconSize(QSize(iconsize, iconsize));

}


namespace {
QString RadioLanguage() {
  if (qApp->property("nasaqLiveLanguage").isValid()) return qApp->property("nasaqLiveLanguage").toString();
  auto language = qEnvironmentVariable("NASAQ_TEST_LANGUAGE");
  if (language.isEmpty()) language = QSettings(QStringLiteral("NASAQ"), QStringLiteral("Preferences")).value(QStringLiteral("language"), QStringLiteral("system")).toString();
  if (language == QStringLiteral("system")) language = QLocale::system().name();
  return language.startsWith(QStringLiteral("ar")) ? QStringLiteral("ar") : QStringLiteral("en");
}
QString RadioText(const char *en, const char *ar) {
  return QString::fromUtf8(RadioLanguage() == QStringLiteral("ar") ? ar : en);
}
QString RadioCategory(const QString &id) {
  static const QHash<QString, QPair<const char *, const char *>> labels = {
    {QStringLiteral("classical"), {"Classical & piano", "كلاسيك وبيانو"}},
    {QStringLiteral("jazz"), {"Jazz", "جاز"}}, {QStringLiteral("bossa"), {"Bossa & island rhythms", "بوسا وإيقاعات الجزر"}},
    {QStringLiteral("lounge"), {"Lounge", "لاونج"}}, {QStringLiteral("chill"), {"Chill & downtempo", "تشيل وإيقاعات هادئة"}},
    {QStringLiteral("ambient"), {"Ambient & relaxation", "استرخاء وأجواء هادئة"}},
    {QStringLiteral("world"), {"World music", "موسيقى عالمية"}}, {QStringLiteral("acoustic"), {"Acoustic & folk", "أكوستيك وفولك"}},
    {QStringLiteral("vocals"), {"Mellow vocals", "غناء هادئ"}}, {QStringLiteral("arabic"), {"Arabic / Teletext", "عربي / تليتكست"}},
    {QStringLiteral("offices"), {"Offices", "شركات ومكاتب"}}, {QStringLiteral("restaurants"), {"Restaurants", "مطاعم"}},
    {QStringLiteral("cafes"), {"Cafes", "مقاهٍ"}}, {QStringLiteral("shops"), {"Shops", "محلات"}},
    {QStringLiteral("hotels"), {"Hotels", "فنادق"}}, {QStringLiteral("wellness"), {"Spa & relaxation", "سبا واسترخاء"}}
  };
  const auto it = labels.constFind(id);
  return it == labels.cend() ? id : RadioText(it.value().first, it.value().second);
}
}

void RadioViewContainer::SetupDiscovery() {
  discovery_ = new QWidget(this);
  discovery_->setObjectName(QStringLiteral("curatedRadio"));
  auto *layout = new QVBoxLayout(discovery_);
  search_ = new QLineEdit(discovery_);
  search_->setClearButtonEnabled(true);
  search_->setObjectName(QStringLiteral("radioSearch"));
  layout->addWidget(search_);
  auto *filters = new QHBoxLayout;
  genre_ = new QComboBox(discovery_);
  venue_ = new QComboBox(discovery_);
  genre_->setObjectName(QStringLiteral("radioGenre"));
  venue_->setObjectName(QStringLiteral("radioVenue"));
  favorites_only_ = new QCheckBox(discovery_);
  favorites_only_->setObjectName(QStringLiteral("radioFavoritesOnly"));
  filters->addWidget(genre_, 1); filters->addWidget(venue_, 1); filters->addWidget(favorites_only_);
  layout->addLayout(filters);
  stations_ = new QListWidget(discovery_);
  stations_->setObjectName(QStringLiteral("curatedStations"));
  stations_->setAlternatingRowColors(true);
  stations_->setWordWrap(true);
  layout->addWidget(stations_, 1);
  description_ = new QLabel(discovery_);
  description_->setWordWrap(true);
  description_->setTextFormat(Qt::PlainText);
  layout->addWidget(description_);
  auto *buttons = new QHBoxLayout;
  play_ = new QPushButton(discovery_); add_ = new QPushButton(discovery_);
  favorite_ = new QPushButton(discovery_); homepage_ = new QPushButton(discovery_);
  buttons->addWidget(play_); buttons->addWidget(add_); buttons->addWidget(favorite_); buttons->addWidget(homepage_);
  layout->addLayout(buttons);
  note_ = new QLabel(discovery_); note_->setWordWrap(true); layout->addWidget(note_);
  ui_->tab_widget->insertTab(0, discovery_, QString());
  ui_->tab_widget->setCurrentIndex(0);
  QFile file(QStringLiteral(":/radios/curated.json"));
  if (file.open(QIODevice::ReadOnly)) catalog_ = QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("stations")).toArray();
  Settings settings;
  favorites_ = settings.value(QStringLiteral("RadioDiscovery/favorites")).toStringList();
  connect(search_, &QLineEdit::textChanged, this, &RadioViewContainer::FilterStations);
  connect(genre_, &QComboBox::currentIndexChanged, this, &RadioViewContainer::FilterStations);
  connect(venue_, &QComboBox::currentIndexChanged, this, &RadioViewContainer::FilterStations);
  connect(favorites_only_, &QCheckBox::toggled, this, &RadioViewContainer::FilterStations);
  connect(stations_, &QListWidget::currentRowChanged, this, &RadioViewContainer::SelectStation);
  connect(stations_, &QListWidget::itemDoubleClicked, this, [this] { OpenStation(true); });
  connect(play_, &QPushButton::clicked, this, [this] { OpenStation(true); });
  connect(add_, &QPushButton::clicked, this, [this] { OpenStation(false); });
  connect(favorite_, &QPushButton::clicked, this, [this] {
    const auto id = SelectedStation().value(QStringLiteral("id")).toString();
    if (id.isEmpty()) return;
    if (favorites_.contains(id)) favorites_.removeAll(id); else favorites_.append(id);
    Settings settings; settings.setValue(QStringLiteral("RadioDiscovery/favorites"), favorites_);
    FilterStations();
  });
  connect(homepage_, &QPushButton::clicked, this, [this] {
    const QUrl url(SelectedStation().value(QStringLiteral("homepage")).toString());
    if (url.scheme() == QStringLiteral("https")) QDesktopServices::openUrl(url);
  });
  TranslateDiscovery();
}

void RadioViewContainer::TranslateDiscovery() {
  if (!discovery_) return;
  const QSignalBlocker g(genre_), v(venue_);
  const auto selected_genre = genre_->currentData().toString(), selected_venue = venue_->currentData().toString();
  genre_->clear(); venue_->clear();
  genre_->addItem(RadioText("All moods", "جميع الأنماط"), QString());
  venue_->addItem(RadioText("All venues", "جميع الأماكن"), QString());
  for (const auto &key : {"arabic", "classical", "jazz", "bossa", "lounge", "chill", "ambient", "world", "acoustic", "vocals"})
    genre_->addItem(RadioCategory(QString::fromUtf8(key)), QString::fromUtf8(key));
  for (const auto &key : {"offices", "restaurants", "cafes", "shops", "hotels", "wellness"})
    venue_->addItem(RadioCategory(QString::fromUtf8(key)), QString::fromUtf8(key));
  genre_->setCurrentIndex(qMax(0, genre_->findData(selected_genre)));
  venue_->setCurrentIndex(qMax(0, venue_->findData(selected_venue)));
  ui_->tab_widget->setTabText(ui_->tab_widget->indexOf(discovery_), RadioText("Selected stations", "محطات مختارة"));
  search_->setPlaceholderText(RadioText("Search stations, styles or places…", "ابحث عن محطة أو نمط أو مكان…"));
  search_->setAccessibleName(RadioText("Search stations", "البحث عن محطة"));
  genre_->setAccessibleName(RadioText("Music style", "النمط الموسيقي"));
  venue_->setAccessibleName(RadioText("Venue", "المكان"));
  favorites_only_->setText(RadioText("Favorites", "المفضلة"));
  play_->setText(RadioText("Play station", "تشغيل المحطة"));
  add_->setText(RadioText("Add to playlist", "إضافة إلى القائمة"));
  homepage_->setText(RadioText("Station website", "موقع المحطة"));
  note_->setText(RadioText("Venue suggestions describe the mood. Public playback rights are not included; check the station's terms. Internet required.", "اقتراح المكان بحسب الأجواء. حقوق التشغيل العام غير مشمولة؛ راجع شروط المحطة. يلزم اتصال بالإنترنت."));
  FilterStations();
}

QJsonObject RadioViewContainer::SelectedStation() const {
  const auto *item = stations_->currentItem();
  return item ? catalog_.at(item->data(Qt::UserRole).toInt()).toObject() : QJsonObject();
}

void RadioViewContainer::FilterStations() {
  const auto selected = SelectedStation().value(QStringLiteral("id")).toString();
  const QSignalBlocker blocker(stations_);
  stations_->clear();
  int selected_row = 0;
  for (int i = 0; i < catalog_.size(); ++i) {
    const auto s = catalog_.at(i).toObject();
    const auto id = s.value(QStringLiteral("id")).toString();
    const auto category = s.value(QStringLiteral("genre")).toString();
    QStringList venues;
    bool venue_matches = venue_->currentData().toString().isEmpty();
    for (const auto &value : s.value(QStringLiteral("venues")).toArray()) {
      venues.append(RadioCategory(value.toString()));
      if (value.toString() == venue_->currentData().toString()) venue_matches = true;
    }
    if (!venue_matches || (!genre_->currentData().toString().isEmpty() && genre_->currentData().toString() != category)) continue;
    if (favorites_only_->isChecked() && !favorites_.contains(id)) continue;
    const auto name = s.value(QStringLiteral("name")).toString();
    const QString details = RadioCategory(category) + QStringLiteral(" · ") + venues.join(QStringLiteral(" / "));
    const QString searchable = name + details + s.value(QStringLiteral("description")).toString() + s.value(QStringLiteral("description_ar")).toString();
    if (!searchable.contains(search_->text().trimmed(), Qt::CaseInsensitive)) continue;
    auto *item = new QListWidgetItem((favorites_.contains(id) ? QStringLiteral("★ ") : QString()) + name + QLatin1Char('\n') + details, stations_);
    item->setData(Qt::UserRole, i);
    item->setToolTip(s.value(QStringLiteral("url")).toString());
    if (id == selected) selected_row = stations_->count() - 1;
  }
  stations_->setCurrentRow(stations_->count() ? selected_row : -1);
  SelectStation();
}

void RadioViewContainer::SelectStation() {
  const auto s = SelectedStation();
  for (auto *button : {play_, add_, favorite_, homepage_}) button->setEnabled(!s.isEmpty());
  const auto key = RadioLanguage() == QStringLiteral("ar") ? QStringLiteral("description_ar") : QStringLiteral("description");
  description_->setText(s.isEmpty() ? RadioText("No stations match these filters.", "لا توجد محطات تطابق البحث.") : s.value(key).toString());
  favorite_->setText(favorites_.contains(s.value(QStringLiteral("id")).toString()) ? RadioText("Remove favorite", "إزالة من المفضلة") : RadioText("Save favorite", "إضافة للمفضلة"));
}

void RadioViewContainer::OpenStation(const bool play) {
  const auto s = SelectedStation();
  const QUrl url(s.value(QStringLiteral("url")).toString());
  if (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http")) return;
  Song song(Song::Source::Stream);
  song.set_valid(true); song.set_filetype(Song::FileType::Stream);
  song.set_title(s.value(QStringLiteral("name")).toString()); song.set_url(url);
  auto *mime = new RadioMimeData;
  mime->songs << song; mime->setUrls({url});
  mime->override_user_settings_ = true; mime->play_now_ = play;
  mime->setProperty("nasaqCuratedRadio", true);
  Q_EMIT AddToPlaylist(mime);
}

void RadioViewContainer::changeEvent(QEvent *event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::LanguageChange || event->type() == QEvent::LayoutDirectionChange) TranslateDiscovery();
}
