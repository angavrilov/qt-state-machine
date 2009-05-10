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

#include "qscriptedstatemachine.h"
#include "spview.h"
#include "spmodel.h"
#include "spengine.h"
#include "spharvester.h"
#include "math.h"
#include "time.h"
#include <QDebug>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QScriptEngine>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QScriptContext>
#include <QScriptEngine>
#include <QMenu>
#include <QMainWindow>


int main( int argc, char **argv)
{
    QApplication app(argc, argv);
    QString dir;
    bool recurse;
    if (argc > 1) {
        dir = QString(argv[1]);
        if (argc > 2)
        {
            recurse = !strcmp(argv[2],"-recurse");
        }
    } else {
        printf("Usage: stateplayer directory [-recurse]");
        return 0;
    }

    app.setApplicationName("SCXML-mediaplayer");

    SPView* view = new SPView(NULL);
    QtScriptedStateMachine *sm = QtScriptedStateMachine::load(":/mediaplayer.scxml");
    QObject::connect (sm, SIGNAL(finished()), &app, SLOT(quit()));
    SPModel* model= new SPModel(NULL);
    view->setModel(model);
    model->setObjectName("model");
    SPEngine* engine = new SPEngine(sm);
    engine->setObjectName("engine");
    SPHarvester* harvester = new SPHarvester (view);
    QObject::connect (harvester, SIGNAL(foundTrack(SongData)), model, SLOT(addSong(SongData)));
    harvester->harvest(dir,recurse);
    view->setObjectName("view");
    sm->registerObject(model);
    sm->registerObject(engine);
    sm->registerObject(view,"",true);
    view->show ();
    sm->start ();
    return app.exec ();
}
