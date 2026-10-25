// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QApplication>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTemporaryDir>
#include "radios/radioviewcontainer.h"
#include "radios/radiomimedata.h"
class RadioDiscoveryTest : public QObject {
  Q_OBJECT
 private Q_SLOTS:
  void discoverFilterAndRestoreFavorites() {
    RadioViewContainer view;
    auto list = view.findChild<QListWidget *>(QStringLiteral("curatedStations"));
    auto search = view.findChild<QLineEdit *>(QStringLiteral("radioSearch"));
    QVERIFY(list); QVERIFY(search); QCOMPARE(list->count(), 18);
    QVERIFY(list->item(0)->text().contains(QStringLiteral("Teletext")));
    search->setText(QStringLiteral("تليتكست")); QCOMPARE(list->count(), 1);
    search->setText(QStringLiteral("no-such-radio")); QCOMPARE(list->count(), 0);
    search->clear(); QCOMPARE(list->count(), 18);
    for (auto button : view.findChildren<QPushButton *>())
      if (button->text() == QStringLiteral("Save favorite")) button->click();
    view.findChild<QCheckBox *>(QStringLiteral("radioFavoritesOnly"))->setChecked(true); QCOMPARE(list->count(), 1);
    RadioViewContainer restored;
    restored.findChild<QCheckBox *>(QStringLiteral("radioFavoritesOnly"))->setChecked(true);
    QCOMPARE(restored.findChild<QListWidget *>(QStringLiteral("curatedStations"))->count(), 1);
  }
  void stylesAndVenuesCombine() {
    RadioViewContainer view;
    auto list = view.findChild<QListWidget *>(QStringLiteral("curatedStations"));
    auto genre = view.findChild<QComboBox *>(QStringLiteral("radioGenre"));
    auto venue = view.findChild<QComboBox *>(QStringLiteral("radioVenue"));
    genre->setCurrentIndex(genre->findData(QStringLiteral("classical"))); QCOMPARE(list->count(), 3);
    venue->setCurrentIndex(venue->findData(QStringLiteral("wellness"))); QCOMPARE(list->count(), 0);
    genre->setCurrentIndex(0); QCOMPARE(list->count(), 1);
    venue->setCurrentIndex(0); QCOMPARE(list->count(), 18);
  }
  void actionsKeepExistingPlaylistAndUseGuardedTransport() {
    RadioViewContainer view;
    int count = 0;
    connect(&view, &RadioViewContainer::AddToPlaylist, this, [&count](QMimeData *raw) {
      auto data = qobject_cast<RadioMimeData *>(raw);
      QVERIFY(data); QCOMPARE(data->songs.size(), 1);
      QCOMPARE(data->songs.first().url(), QUrl(QStringLiteral("https://radio.teletext-ar.tech/listen/teletext/radio.mp3")));
      QVERIFY(data->override_user_settings_); QVERIFY(!data->clear_first_);
      QVERIFY(data->property("nasaqCuratedRadio").toBool());
      QCOMPARE(data->play_now_, count == 0);
      ++count; delete data;
    });
    for (const auto &label : {QStringLiteral("Play station"), QStringLiteral("Add to playlist")})
      for (auto button : view.findChildren<QPushButton *>()) if (button->text() == label) button->click();
    QCOMPARE(count, 2);
  }
  void liveArabicTranslation() {
    RadioViewContainer view;
    qApp->setProperty("nasaqLiveLanguage", QStringLiteral("ar"));
    QEvent event(QEvent::LanguageChange); QApplication::sendEvent(&view, &event);
    auto search = view.findChild<QLineEdit *>(QStringLiteral("radioSearch"));
    QVERIFY(search->placeholderText().contains(QStringLiteral("ابحث")));
    search->setText(QStringLiteral("فنادق"));
    QVERIFY(view.findChild<QListWidget *>(QStringLiteral("curatedStations"))->count() > 0);
    qApp->setProperty("nasaqLiveLanguage", QStringLiteral("en"));
  }
};
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  Q_INIT_RESOURCE(data);
  QTemporaryDir settings;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
  app.setOrganizationName(QStringLiteral("NASAQ-Test")); app.setApplicationName(QStringLiteral("RadioDiscovery"));
  app.setProperty("nasaqLiveLanguage", QStringLiteral("en"));
  RadioDiscoveryTest test; return QTest::qExec(&test, argc, argv);
}
#include "radio_discovery_test.moc"
