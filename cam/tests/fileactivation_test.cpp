// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QApplication>
#include "playlist/playlist.h"
#include "playlist/songplaylistitem.h"

class FileActivationTest : public QObject {
  Q_OBJECT
 private Q_SLOTS:
  void reuseExistingPathWithoutChangingPlaybackOrRemovingIntentionalCopies() {
    Playlist playlist(nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    Song song;
    song.set_url(QUrl::fromLocalFile(QStringLiteral("C:/Music/track.wav")));
    playlist.InsertItems({PlaylistItemPtr(new SongPlaylistItem(song)), PlaylistItemPtr(new SongPlaylistItem(song))});
    QSignalSpy play(&playlist, &Playlist::PlayRequested);
    for (int i = 0; i < 10; ++i)
      QCOMPARE(playlist.OpenUrlsUnique({song.url()}, true), 0);
    QCOMPARE(playlist.rowCount(), 2);
    QCOMPARE(play.count(), 0); // Caller decides whether playback is already active or held.
  }
  void normalizedWindowsPathFindsExistingFile() {
    Playlist playlist(nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    Song song;
    song.set_url(QUrl::fromLocalFile(QStringLiteral("C:/Music/track.wav")));
    playlist.InsertItems({PlaylistItemPtr(new SongPlaylistItem(song))});
    QCOMPARE(playlist.OpenUrlsUnique({QUrl::fromLocalFile(QStringLiteral("c:/MUSIC/track.wav"))}, false), 0);
    QCOMPARE(playlist.rowCount(), 1);
  }
};
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("NASAQ-Test"));
  app.setApplicationName(QStringLiteral("FileActivation-Tests"));
  FileActivationTest test;
  return QTest::qExec(&test, argc, argv);
}
#include "fileactivation_test.moc"
