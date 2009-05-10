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

#include <QtCore>
#include <stdio.h>
#ifdef QT_STATEMACHINE_SOLUTION
#include <qstatemachine.h>
#include <qstate.h>
#include <qtransition.h>
#endif

class ClockEvent : public QEvent
{
public:
    ClockEvent() : QEvent(QEvent::Type(QEvent::User+2))
        {}
};

class ClockState : public QtState
{
public:
    ClockState(QtStateMachine *machine, QtState *parent)
        : QtState(parent), m_machine(machine) {}

protected:
    virtual void onEntry()
    {
        fprintf(stdout, "ClockState entered; posting the initial tick\n");
        m_machine->postEvent(new ClockEvent());
    }

private:
    QtStateMachine *m_machine;
};

class ClockTransition : public QtAbstractTransition
{
public:
    ClockTransition(QtStateMachine *machine)
        : QtAbstractTransition(), m_machine(machine) { }

protected:
    virtual bool eventTest(QEvent *e) const {
        return (e->type() == QEvent::User+2);
    }
    virtual void onTransition()
    {
        fprintf(stdout, "ClockTransition triggered; posting another tick with a delay of 1 second\n");
        m_machine->postEvent(new ClockEvent(), 1000);
    }

private:
    QtStateMachine *m_machine;
};

class ClockListener : public QtAbstractTransition
{
public:
    ClockListener()
        : QtAbstractTransition() {}

protected:
    virtual bool eventTest(QEvent *e) const {
        return (e->type() == QEvent::User+2);
    }
    virtual void onTransition()
    {
        fprintf(stdout, "ClockListener heard a tick!\n");
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QtStateMachine machine;
    QtState *group = new QtState(QtState::ParallelGroup);
    group->setObjectName("group");

    ClockState *clock = new ClockState(&machine, group);
    clock->setObjectName("clock");
    clock->addTransition(new ClockTransition(&machine));

    QtState *listener = new QtState(group);
    listener->setObjectName("listener");
    listener->addTransition(new ClockListener());

    machine.addState(group);
    machine.setInitialState(group);
    machine.start();

    return app.exec();
}
