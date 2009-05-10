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

#include "spengine.h"
#include <phonon>

using namespace Phonon;

class SPEnginePvt
{
    public:
    MediaObject* mediaObject;
    AudioOutput* audioOutput;
};
void SPEngine::clearQueue()
{
    pvt->mediaObject->clearQueue();
}

int SPEngine::currentTime() const 
{
    return pvt->mediaObject->currentTime ();
}
int SPEngine::totalTime() const 
{
    return pvt->mediaObject->totalTime();
}

void SPEngine::enqueue (const QUrl & u)
{
    pvt->mediaObject->enqueue(MediaSource(u));
}
void SPEngine::setTrack(const QUrl & u)
{
    pvt->mediaObject->setCurrentSource(MediaSource(u));
}
void SPEngine::play()
{
    pvt->mediaObject->play ();
}

void SPEngine::pause()
{
    pvt->mediaObject->pause ();
}

void SPEngine::stop()
{
    pvt->mediaObject->stop();
}

void SPEngine::seek(qint64 pos)
{
    pvt->mediaObject->seek(pos);
}

void SPEngine::setVolume(int v)
{
    pvt->audioOutput->setVolume((qreal)v/100);
}

void SPEngine::onVolumeChanged(qreal r)
{
    emit volumeChanged(r*100);
}
int SPEngine::volume() const
{
    return pvt->audioOutput->volume()*100;
}

SPEngine::SPEngine(QObject* p) : QObject(p)
{
    pvt = new SPEnginePvt;
    pvt->mediaObject = new Phonon::MediaObject(this);
    pvt->audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    createPath (pvt->mediaObject, pvt->audioOutput);
    pvt->mediaObject->setTickInterval(500);
    connect (pvt->mediaObject, SIGNAL(aboutToFinish()), this, SIGNAL(aboutToFinish()));
    connect (pvt->mediaObject, SIGNAL(tick(qint64)), this, SIGNAL(tick(qint64)));
    connect (pvt->mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SIGNAL(totalTimeChanged(qint64)));
    connect (pvt->audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(onVolumeChanged(qreal)));
}

SPEngine::~SPEngine ()
{
    delete pvt;
}
