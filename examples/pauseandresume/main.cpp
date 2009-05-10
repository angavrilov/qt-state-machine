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

#include <QtGui>
#ifdef QT_STATEMACHINE_SOLUTION
#include <qstatemachine.h>
#include <qstate.h>
#include <qfinalstate.h>
#include <qhistorystate.h>
#endif

class Window : public QWidget
{
public:
    Window(QWidget *parent = 0)
        : QWidget(parent)
    {
        QPushButton *pb = new QPushButton("Go");
        QPushButton *pauseButton = new QPushButton("Pause");
        QPushButton *quitButton = new QPushButton("Quit");
        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->addWidget(pb);
        vbox->addWidget(pauseButton);
        vbox->addWidget(quitButton);

        QtStateMachine *machine = new QtStateMachine(this);

        QtState *process = new QtState(machine->rootState());
        process->setObjectName("process");

        QtState *s1 = new QtState(process);
        s1->setObjectName("s1");
        QtState *s2 = new QtState(process);
        s2->setObjectName("s2");
        s1->addTransition(pb, SIGNAL(clicked()), s2);
        s2->addTransition(pb, SIGNAL(clicked()), s1);

        QtHistoryState *h = process->addHistoryState();
        h->setDefaultState(s1);

        QtState *interrupted = new QtState(machine->rootState());
        interrupted->setObjectName("interrupted");
        QtFinalState *terminated = new QtFinalState(machine->rootState());
        terminated->setObjectName("terminated");
        interrupted->addTransition(pauseButton, SIGNAL(clicked()), h);
        interrupted->addTransition(quitButton, SIGNAL(clicked()), terminated);

        process->addTransition(pauseButton, SIGNAL(clicked()), interrupted);
        process->addTransition(quitButton, SIGNAL(clicked()), terminated);

        process->setInitialState(s1);
        machine->setInitialState(process);
        QObject::connect(machine, SIGNAL(finished()), QApplication::instance(), SLOT(quit()));
        machine->start();
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window win;
    win.show();
    return app.exec();
}
