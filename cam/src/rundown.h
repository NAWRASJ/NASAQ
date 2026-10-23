// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDateTime>
#include <QList>
#include <QStringList>
struct RundownResult{qint64 total=0,remaining=0;int unknown=0;QStringList ends;};
inline QString RundownDuration(qint64 ms){qint64 seconds=qMax(qint64(0),ms)/1000;return QStringLiteral("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0')).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));}
inline RundownResult CalculateRundown(const QList<qint64> &durations,int current,qint64 position,const QDateTime &now,bool ordered=true){
 RundownResult r;current=qBound(0,current,qMax(0,int(durations.size())-1));bool uncertain=!ordered;auto end=now;
 for(int i=0;i<durations.size();++i){auto length=durations[i];if(length<=0)++r.unknown;else r.total+=length;if(i<current){r.ends<<QStringLiteral("—");continue;}if(length<=0)uncertain=true;auto remaining=qMax(qint64(0),length-(i==current?qMax(qint64(0),position):0));r.remaining+=remaining;end=end.addMSecs(remaining);r.ends<<(uncertain?QStringLiteral("—"):end.toString(end.date()==now.date()?QStringLiteral("HH:mm:ss"):QStringLiteral("yyyy-MM-dd HH:mm:ss")));}return r;
}
