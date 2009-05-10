/****************************************************************************
**
** This file is part of a Qt Solutions component.
** 
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
** 
** Contact:  Qt Software Information (qt-info@nokia.com)
** 
** Commercial Usage  
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
** 
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
** 
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
** 
** GNU General Public License Usage 
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** 
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
** 
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** 
****************************************************************************/

#include "spharvester.h"
#include <QDir>
#include <phonon>
#include <QQueue>
#include <QFile>
#include <QUrl>

using namespace Phonon;

struct SPHarvesterPvt
{
    MediaObject* mediaObject;
    QQueue<QString> pathQueue;
};

SPHarvester::SPHarvester(QObject* o) : QObject(o)
{
    pvt = new SPHarvesterPvt;
    pvt->mediaObject = new MediaObject(this);
    connect (pvt->mediaObject, SIGNAL(metaDataChanged()), this, SLOT(readMetaData ()));
}

SPHarvester::~SPHarvester()
{
    delete pvt;
}

void SPHarvester::harvest (const QString & directory, bool recurse)
{
    QDir d (directory);
    QFileInfoList l = d.entryInfoList(QStringList() << "*.mp3",recurse ? QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files : QDir::Files);
    foreach (QFileInfo fi, l) {
        if (fi.isDir()) {
            harvest (fi.absoluteFilePath(), recurse);
        } else {
            pvt->pathQueue.enqueue(fi.absoluteFilePath());
        }
    }
    harvestNext ();
}

void SPHarvester::harvestNext ()
{
    if (pvt->pathQueue.empty())
        emit done();
    else {
        QString s = pvt->pathQueue.dequeue();
        pvt->mediaObject->setCurrentSource(MediaSource(s));
    }
}

void SPHarvester::readMetaData ()
{
    QStringList albums = pvt->mediaObject->metaData("ALBUM");
    QStringList titles = pvt->mediaObject->metaData("TITLE");
    QStringList artists = pvt->mediaObject->metaData("ARTIST");
    QStringList trackNums = pvt->mediaObject->metaData("TRACKNUMBER");
    SongData sd;
    sd.url = pvt->mediaObject->currentSource().url().toString();
    sd.album = albums.count() ? albums[0] : "Unknown Album";
    sd.artist = artists.count() ? artists[0] : "Unknown Artist";
    sd.trackNumber = trackNums.count() ? trackNums[0].toInt() : 0;
    sd.genres = pvt->mediaObject->metaData("GENRE");
    sd.title = titles.count() ? titles[0] : QFileInfo(sd.url).baseName();

    emit foundTrack(sd);
    harvestNext ();
}
