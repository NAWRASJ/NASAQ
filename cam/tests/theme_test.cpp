// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QSettings>
#include <QVBoxLayout>
#include "widgets/playingwidget.h"
#include "utilities/nasaqvinyl.h"
#include "core/iconloader.h"
#include "utilities/strutils.h"
#include "core/appearance.h"
#include "nasaqtheme.h"
#include "analyzer/blockanalyzer.h"
#include <limits>
class SpectrumProbe : public BlockAnalyzer {
 public:
 SpectrumProbe():BlockAnalyzer(nullptr){}
 QImage frame(bool playing, const std::vector<float>& samples, bool advance=true){
 is_playing_=playing; QImage image(size(),QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);QPainter painter(&image);analyze(painter,samples,advance);return image;
 }
};
class ThemeTest : public QObject {
 Q_OBJECT
 private Q_SLOTS:
 void spectrumRespondsAndSettles(){
  ApplyNasaqTheme();SpectrumProbe spectrum;spectrum.resize(320,90);
  std::vector<float> signal(256,0.0F);for(int i=1;i<256;++i)signal[i]=float((i*17)%41)/50.0F;
  auto silent=spectrum.frame(false,{});
  for(int i=0;i<15;++i)spectrum.frame(true,signal);
  auto active=spectrum.frame(true,signal,false);QVERIFY(active!=silent);
  QCOMPARE(spectrum.frame(true,signal,false),active);
  if(!qEnvironmentVariable("NASAQ_SPECTRUM_EVIDENCE").isEmpty())active.save(qEnvironmentVariable("NASAQ_SPECTRUM_EVIDENCE"));
  for(int i=0;i<100;++i)spectrum.frame(false,signal);
  QCOMPARE(spectrum.frame(false,signal,false),silent);
  // Stale PCM during a prayer pause must not restart the visualizer.
  for(int i=0;i<20;++i)QCOMPARE(spectrum.frame(false,signal),silent);
  std::vector<float> invalid(256,std::numeric_limits<float>::quiet_NaN());
  QCOMPARE(spectrum.frame(true,invalid),silent);
  QCOMPARE(spectrum.frame(true,{}),silent);
  spectrum.resize(159,9);QVERIFY(!spectrum.frame(false,{}).isNull());
 }
 void urlLogsRemoveCredentials(){
  QCOMPARE(Utilities::UrlForLog(QUrl(QStringLiteral("https://user:secret@example.com/audio?token=secret#access_token=secret"))),QStringLiteral("https://example.com/audio"));
 }
 void brandKeepsWebsiteColors(){
  auto icon=IconLoader::Load(QStringLiteral("strawberry"));
  for(int size:{16,32,64,128}){
    auto image=icon.pixmap(size,size).toImage();QVERIFY(!image.isNull());
    // Sample the solid lower tile, away from the resampled mark at 16 px.
    const QColor background=image.pixelColor(image.width()/2,image.height()*7/8);
    QVERIFY(background.alpha()>240);QVERIFY(qAbs(background.red()-25)<5);QVERIFY(qAbs(background.green()-63)<5);
    const QColor mark=image.pixelColor(image.width()/2,image.height()*3/10);QVERIFY(mark.red()>210);QVERIFY(mark.green()>215);
  }
 }
 void paletteAndIcons(){
  ApplyNasaqTheme();Appearance::SetCustomPaletteColors({{QPalette::Window,QColor("#0b1113")}});QCOMPARE(qApp->palette().color(QPalette::Window),QColor("#f8f7f2"));
  auto icon=IconLoader::Load(QStringLiteral("media-playback-start"));QVERIFY(!icon.isNull());
  QVERIFY(icon.pixmap(32,32,QIcon::Normal).toImage()!=icon.pixmap(32,32,QIcon::Selected).toImage());
 }
 void vinylOnlyMovesWithoutCoverWhilePlaying(){
  QWidget host;QVBoxLayout layout(&host);PlayingWidget widget;layout.addWidget(&widget);host.resize(280,380);host.show();widget.SetEnabled();QTest::qWait(50);
  bool allowed=true;widget.SetPlaybackProbe([&]{return allowed;});
  Song song;song.set_valid(true);song.set_title(QStringLiteral("Vinyl test"));song.set_url(QUrl::fromLocalFile(QStringLiteral("C:/test.wav")));
  widget.SongChanged(song);widget.SetFallbackCover(true);widget.AlbumCoverLoaded(song,NasaqVinylImage());widget.show();
  QTest::qWait(1200);widget.SetPlaybackActive(true);auto before=widget.grab().toImage();QTest::qWait(250);if(!qEnvironmentVariable("NASAQ_THEME_EVIDENCE").isEmpty())before.save(qEnvironmentVariable("NASAQ_THEME_EVIDENCE"));QVERIFY(!before.isNull());QVERIFY(before!=widget.grab().toImage());
  widget.SetPlaybackActive(false);auto paused=widget.grab().toImage();QTest::qWait(160);QCOMPARE(widget.grab().toImage(),paused);
  widget.SetPlaybackActive(true);QTest::qWait(100);allowed=false;QTest::qWait(80);auto held=widget.grab().toImage();QTest::qWait(160);QCOMPARE(widget.grab().toImage(),held);
  allowed=true;widget.SetPlaybackActive(true);QTest::qWait(160);QVERIFY(held!=widget.grab().toImage());
  QImage cover(200,200,QImage::Format_RGB32);cover.fill(QColor("#a48c70"));
  widget.SetFallbackCover(false);widget.AlbumCoverLoaded(song,cover);QTest::qWait(1200);auto actual=widget.grab().toImage();QTest::qWait(160);QCOMPARE(widget.grab().toImage(),actual);
 }
};
int main(int argc,char **argv){
 QApplication app(argc,argv);QCoreApplication::setOrganizationName(QStringLiteral("NASAQ-ThemeTests"));QCoreApplication::setApplicationName(QStringLiteral("Theme"));
 QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
 Q_INIT_RESOURCE(data);ThemeTest test;return QTest::qExec(&test,argc,argv);
}
#include "theme_test.moc"

