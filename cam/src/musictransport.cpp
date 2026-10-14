// SPDX-License-Identifier: GPL-3.0-or-later
#include "musictransport.h"
#include <QFileInfo>
QtMusicTransport::QtMusicTransport(QObject*p):MusicTransport(p){
 player.setAudioOutput(&output);output.setVolume(.5);
 connect(&player,&QMediaPlayer::mediaStatusChanged,this,[this](auto s){if(s==QMediaPlayer::EndOfMedia)next();});
 connect(&player,&QMediaPlayer::playbackStateChanged,this,[this](auto s){if(s==QMediaPlayer::PlayingState){if(suspended){player.pause();return;}Q_EMIT event(QStringLiteral("play_started"),player.source().toLocalFile());}else if(s==QMediaPlayer::StoppedState)Q_EMIT event(QStringLiteral("play_stopped"),player.source().toLocalFile());});
 connect(&player,&QMediaPlayer::errorOccurred,this,[this](auto,const QString&e){Q_EMIT event(QStringLiteral("audio_error"),e);});
}
bool QtMusicTransport::replace(const QStringList&list){if(list.isEmpty())return false;for(const auto&f:list)if(!QFileInfo(f).isFile())return false;files=list;index=-1;next();return true;}
void QtMusicTransport::next(){if(files.isEmpty())return;index=(index+1)%files.size();player.setSource(QUrl::fromLocalFile(files[index]));if(!suspended)player.play();}
bool QtMusicTransport::playing()const{return player.playbackState()==QMediaPlayer::PlayingState;}
void QtMusicTransport::suspend(bool active){suspended=active;if(active)player.pause();}
void QtMusicTransport::resume(){if(!suspended)player.play();}
void QtMusicTransport::stop(){files.clear();player.stop();}
