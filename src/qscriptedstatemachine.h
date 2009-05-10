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


#ifndef QSCRIPTEDSTATEMACHINE_H
#define QSCRIPTEDSTATEMACHINE_H
#ifndef QT_NO_SCRIPT
#include "qstatemachine.h"
#include "qstateaction.h"
#include "qtransition.h"
#include "qstate.h"
#include <QVariant>
#include <QEvent>
#include <QStringList>
#include <QScriptValue>
#include <QUrl>

#ifndef Q_SCTOOLS_EXPORT
#define Q_SCTOOLS_EXPORT Q_STATEMACHINE_CORE_EXPORT
#endif

class QScriptEngine;
class QtScriptedStateMachine;
class QSignalMapper;
class Q_SCTOOLS_EXPORT QtScriptedEvent : public QEvent
{
    public:
        static QEvent::Type eventType();
        QString eventName() const;
        QStringList paramNames () const;
        QVariantList paramValues () const;
        QScriptValue content () const;
        QVariant param (const QString & name) const;
        QtScriptedEvent (
                const QString & name,
                const QStringList & paramNames = QStringList(),
                const QVariantList & paramValues = QVariantList(),
                const QScriptValue & content = QScriptValue());

        struct MetaData
        {
            QUrl origin,target;
            QString originType, targetType;
            QString invokeID;
            enum Kind { Platform, Internal, External } kind;
        };

        MetaData metaData;

    private:
        QString ename;
        QStringList pnames;
        QVariantList pvals;
        QScriptValue cnt;
};


class Q_SCTOOLS_EXPORT QtScriptedTransition : public QtTransition
{
    Q_OBJECT
    Q_PROPERTY(QString conditionExpression READ conditionExpression WRITE setConditionExpression)
    Q_PROPERTY(QString eventPrefix READ eventPrefix WRITE setEventPrefix)

    public:
        QtScriptedTransition (QtState* state, QtScriptedStateMachine* machine);

        QString conditionExpression () const { return cond; }
        void setConditionExpression (const QString & c) { cond = c; }
        QString eventPrefix () const { return ev; }
        void setEventPrefix (const QString & e) { ev = e; }

    protected:
        bool eventTest(QEvent*) const;
    private:
        QtScriptedStateMachine* ssm;
        QString ev,cond;
};

class Q_SCTOOLS_EXPORT QtSsmInvoker : public QObject
{
    Q_OBJECT

    protected:
        QtSsmInvoker(QtScriptedEvent* ievent, QtStateMachine* p) : QObject(p), initEvent(ievent) {}

    public:
        virtual ~QtSsmInvoker();

    public Q_SLOTS:
        virtual void activate() = 0;
        virtual void cancel() { deleteLater(); }

    protected Q_SLOTS:
        void postParentEvent (const QString & event);

    protected:
        QtScriptedStateMachine* parentStateMachine() { return (QtScriptedStateMachine*)parent(); }
        void postParentEvent (QtScriptedEvent* ev);
        QtScriptedEvent* initEvent;

    friend struct QtScriptedStateMachineFunctions;
};

struct Q_SCTOOLS_EXPORT QtSsmInvokerFactory
{
    virtual QtSsmInvoker* createInvoker (QtScriptedEvent* event, QtScriptedStateMachine* stateMachine) = 0;
    virtual bool isTypeSupported (const QString & type) const = 0;
    virtual void init (QtScriptedStateMachine*) = 0;
};

template <class T>
class Q_SCTOOLS_EXPORT QtSsmAutoInvokerFactory : public QtSsmInvokerFactory
{
    QtSsmInvoker* createInvoker (QtScriptedEvent* _e, QtScriptedStateMachine* _sm) { return new T(_e,_sm); }
    bool isTypeSupported(const QString & _s) const { return T::isTypeSupported(_s); }
    void init (QtScriptedStateMachine* sm) { T::initInvokerFactory(sm); }
};


class Q_SCTOOLS_EXPORT QtScriptedStateMachine : public QtStateMachine
{
    Q_OBJECT

    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl)


    public:
        QtScriptedStateMachine(QObject* o = NULL);
        virtual ~QtScriptedStateMachine();
    protected:
        // overloaded to store the event for the script environment's use (_event), and to convert
        // StateFinished events to "done." named events
        virtual void beginSelectTransitions(QEvent*);
        virtual void endMicrostep(QEvent*);

    public:
        QScriptEngine* scriptEngine () const;
        void registerObject (QObject* object, const QString & name = QString(), bool recursive = false);
        void registerInvokerFactory (QtSsmInvokerFactory* f);
        void setBaseUrl (const QUrl &);
        QUrl baseUrl () const;
        static QtScriptedStateMachine* load (const QString & filename, QObject* o = NULL);

    public Q_SLOTS:
        void postNamedEvent(const QString &);
        void executeScript (const QString &);

    private Q_SLOTS:
        void registerSession();
        void unregisterSession();

    Q_SIGNALS:
        void eventTriggered(const QString &);



    private:
        class QtScriptedStateMachinePrivate* pvt;
        friend class QtScStreamLoader;
        friend struct QtScriptedStateMachineFunctions;
};

#endif
#endif // QtScriptedSTATEMACHINE_H
