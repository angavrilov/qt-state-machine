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

/*!
  \class QtScriptedStateMachine
  \reentrant

  \brief The QtScriptedStateMachine class provides a way to use scripting with the Qt State Machine Framework.

  \ingroup sctools

  Though can be used alone, QtScriptedStateMachine is mainly a runtime helper to using the
  state-machine framework with SCXML files.


  \sa QtStateMachine
*/

#include "qscriptedstatemachine.h"
#include <QScriptEngine>
#include <QScriptValueIterator>
#include "qstatefinishedevent.h"
#include <QDebug>
#include <QTimer>
#include <QSignalMapper>
#include <QUuid>
#include <QHash>
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QStack>
#include "qhistorystate.h"
#include "qabstracttransition_p.h"
#include "qfinalstate.h"
#include "qabstractstate.h"
#ifdef QT_GUI_LIB
#include "qssmguiinvokers_p.h"
#endif


class QtScriptedStateMachinePrivate
{
    public:

        enum { MaxSnapshots = 200};

        struct AnchorSnapshot
        {
            QtAbstractState* state;
            QString location;
            QScriptValue snapshot;
            QString anchorType;
        };


        QScriptEngine* scriptEng;
        QList<QtSsmInvokerFactory*> invokerFactories;
        QUrl burl;
        QString sessionID;
        QString startScript;


        QStack<AnchorSnapshot> snapshotStack;
        QMultiHash<QString,QtTransition*> anchorTransitions;
        QHash<QString,AnchorSnapshot> curSnapshot;


        static QHash<QString,QtScriptedStateMachine*> sessions;
};
QHash<QString,QtScriptedStateMachine*> QtScriptedStateMachinePrivate::sessions;

class QtSsmTimer : public QObject
{
	Q_OBJECT
	public:
        QtSsmTimer(QScriptEngine* engine, const QScriptValue & scr, int delay) : QObject(engine),script(scr)
        {
                startTimer(delay);
        }
	protected:
        void timerEvent(QTimerEvent*)
		{
			if (script.isFunction())
				script.call();
			else if (script.isString())
				script.engine()->evaluate(script.toString());
		}
		
	private:
                QScriptValue script;
	
};

static QScriptValue _q_deepCopy(const QScriptValue & val)
{
    if (val.isObject() || val.isArray()) {
        QScriptValue v = val.isArray() ? val.engine()->newArray() : val.engine()->newObject();
        v.setData(val.data());
        QScriptValueIterator it (val);
        while (it.hasNext()) {
            it.next();
            v.setProperty(it.name(), _q_deepCopy(it.value()));
        }
        return v;
    } else
        return val;
}


struct QtScriptedStateMachineFunctions
{
static QScriptValue cssTime(QScriptContext *context, QScriptEngine *engine)
{
	QString str;
	if (context->argumentCount() > 0)
		str = context->argument(0).toString();
    if (str == "") {
        return qScriptValueFromValue<int>(engine,0);
    }
    else if (str.endsWith("ms")) {
        return qScriptValueFromValue<int>(engine,(str.left(str.length()-2).toInt()));
    }
    else if (str.endsWith("s")) {
        return qScriptValueFromValue<int>(engine,(str.left(str.length()-1).toInt())*1000);
    }
    else {
        return qScriptValueFromValue<int>(engine, (str.toInt()));
    }
}
static QScriptValue setTimeout(QScriptContext *context, QScriptEngine *engine)
{
        if (context->argumentCount() < 2)
                return QScriptValue();
        int timeout = context->argument(1).toInt32();
        QtSsmTimer* tmr = new QtSsmTimer(engine,context->argument(0),timeout);
        return engine->newQObject(tmr);
}
static QScriptValue script_print(QScriptContext *context, QScriptEngine *)
{
    if (context->argumentCount() > 0)
        qDebug() << context->argument(0).toString();
    return QScriptValue();
}
static QScriptValue clearTimeout(QScriptContext *context, QScriptEngine *)
{
        if (context->argumentCount() > 0) {
		QObject* obj = context->argument(0).toQObject();
		obj->deleteLater();
	}
        return QScriptValue();
}

static QScriptValue deepCopy(QScriptContext *context, QScriptEngine *)
{
    if (context->argumentCount() == 0)
        return QScriptValue();
    else
        return _q_deepCopy(context->argument(0));
}

static QScriptValue receiveSignal(QScriptContext *context, QScriptEngine *engine)
{
    QString eventName = context->thisObject().property("e").toString();
    if (!eventName.isEmpty()) {
        QtScriptedStateMachine* ssm = qobject_cast<QtScriptedStateMachine*>(engine->globalObject().property("SMUTIL_stateMachine").toQObject());
        if (ssm) {
            QStringList pnames;
            QVariantList pvals;
            for (int i=0; i < context->argumentCount(); ++i) {
                pnames << QString::number(i);
                pvals << context->argument(i).toVariant();
            }
            QtScriptedEvent* ev = new QtScriptedEvent(eventName,pnames,pvals,QScriptValue());
            ev->metaData.kind = QtScriptedEvent::MetaData::Platform;
            ssm->postEvent(ev);
        }
    }
    return QScriptValue();
}

static QScriptValue postEvent(QScriptContext *context, QScriptEngine *engine)
{
    QtScriptedStateMachine* ssm = qobject_cast<QtScriptedStateMachine*>(engine->globalObject().property("SMUTIL_stateMachine").toQObject());
    if (ssm) {
        QString eventName,target,type;
        QStringList pnames;
        QVariantList pvals;
        QScriptValue cnt;
        if (context->argumentCount() > 0)
            eventName = context->argument(0).toString();
        if (context->argumentCount() > 1)
            target = context->argument(1).toString();
        if (context->argumentCount() > 2)
            type = context->argument(2).toString();

        if (!eventName.isEmpty() || !target.isEmpty()) {
            if (context->argumentCount() > 3)
                qScriptValueToSequence<QStringList>(context->argument(3),pnames);
            if (context->argumentCount() > 4) {
                QScriptValueIterator it (context->argument(4));
                while (it.hasNext()) {
                    it.next();
                    pvals.append(it.value().toVariant());
                }
            } if (context->argumentCount() > 5)
                cnt = context->argument(5);
            QtScriptedEvent* ev = new QtScriptedEvent(eventName,pnames,pvals,cnt);
            if (type == "scxml" || type == "") {
                bool ok = true;
                if (target == "_internal") {
                    ev->metaData.kind = QtScriptedEvent::MetaData::Internal;
                    ssm->postInternalEvent(ev);
                } else if (target == "scxml" || target == "") {
                    ev->metaData.kind = QtScriptedEvent::MetaData::External;
                    ssm->postEvent(ev);
                } else if (target == "_parent") {
                    QtSsmInvoker* p = qobject_cast<QtSsmInvoker*>(ssm->parent());
                    if (p)
                        p->postParentEvent(ev);
                    else
                        ok = false;
                } else {
                    QtScriptedStateMachine* session = QtScriptedStateMachinePrivate::sessions[target];
                    if (session) {
                        session->postEvent(ev);
                    } else
                        ok = false;
                }
                if (!ok)
                    ssm->postNamedEvent("error.targetunavailable");

            } else {
                ssm->postNamedEvent("error.send.typeinvalid");
            }
        }
    }
    return QScriptValue();
}

// SMUTIL_invoke (type, target, paramNames, paramValues, content)
static QScriptValue invoke(QScriptContext *context, QScriptEngine *engine)
{
    QtScriptedStateMachine* ssm = qobject_cast<QtScriptedStateMachine*>(engine->globalObject().property("SMUTIL_stateMachine").toQObject());
    if (ssm) {
        QString type,target;
        QStringList pnames;
        QVariantList pvals;
        QScriptValue cnt;
        if (context->argumentCount() > 0)
            type = context->argument(0).toString();
        if (type.isEmpty())
            type = "scxml";
        if (context->argumentCount() > 1)
            target = context->argument(1).toString();
        if (context->argumentCount() > 2)
            qScriptValueToSequence<QStringList>(context->argument(2),pnames);
        if (context->argumentCount() > 3) {
                QScriptValueIterator it (context->argument(3));
                while (it.hasNext()) {
                    it.next();
                    pvals.append(it.value().toVariant());
                }
        } if (context->argumentCount() > 4)
                cnt = context->argument(4);
					
					

        QtSsmInvokerFactory* invf = NULL;
        for (int i=0; i < ssm->pvt->invokerFactories.count() && invf == NULL; ++i)
            if (ssm->pvt->invokerFactories[i]->isTypeSupported(type))
                invf = ssm->pvt->invokerFactories[i];
        if (invf) {
            QtScriptedEvent* ev = new QtScriptedEvent("",pnames,pvals,cnt);
            ev->metaData.origin = ssm->baseUrl();
            ev->metaData.target = target;
            ev->metaData.targetType = type;
            ev->metaData.originType = "scxml";
            ev->metaData.kind = QtScriptedEvent::MetaData::External;
            QtSsmInvoker* inv = invf->createInvoker(ev,ssm);
			if (inv)
				inv->activate();
            return engine->newQObject(inv);
        } else {
            ssm->postNamedEvent("error.invalidtargettype");
        }

    }
    return QScriptValue();
}


static QScriptValue isInState(QScriptContext *context, QScriptEngine *engine)
{
    QtScriptedStateMachine* ssm = qobject_cast<QtScriptedStateMachine*>(engine->globalObject().property("SMUTIL_stateMachine").toQObject());
    if (ssm) {
        if (context->argumentCount() > 0) {
            QString name = context->argument(0).toString();
            if (!name.isEmpty()) {
                QSet<QtAbstractState*> cfg = ssm->configuration();
                foreach (QtAbstractState* st, cfg) {
                    if (st->objectName() == name)
                        return qScriptValueFromValue<bool>(engine,true);
                }
            }
        }
    }
    return qScriptValueFromValue<bool>(engine,false);

}

};
/*!
  \class QtScriptedEvent
  \brief The QtScriptedEvent class stands for a general named event with a list of parameter names and parameter values.

  Encapsulates an event that conforms to the SCXML definition of events.

  \ingroup sctools

*/
/*! \enum QtScriptedEvent::MetaData::Kind

    This enum specifies the kind (or context) of the event.
    \value Platform     An event coming from the	 itself, such as a script error.
    \value Internal     An event sent with a <raise> or <send target="_internal">.
    \value External     An event sent from an invoker, directly from C++, or from a <send target="scxml"> element.
*/

/*!
  Returns the name of the event.
  */
  QString QtScriptedEvent::eventName() const
{
    return ename;
}
  /*!
    Return a list containing the parameter names.
    */
QStringList QtScriptedEvent::paramNames () const
{
    return pnames;
}
  /*!
    Return a list containing the parameter values.
    */
QVariantList QtScriptedEvent::paramValues () const
{
    return pvals;
}
  /*!
    Return a QtScript object that can be passed as an additional parameter.
    */
QScriptValue QtScriptedEvent::content () const
{
    return cnt;
}
  /*!
    Returns the parameter value equivalent to parameter \a name.
    */
QVariant QtScriptedEvent::param (const QString & name) const
{
    int idx = pnames.indexOf(name);
    if (idx >= 0)
        return pvals[idx];
    else
        return QVariant();
}
/*!
  Creates a QtScriptedEvent named \a name, with parameter names \a paramNames, parameter values \a paramValues, and
  a QtScript object \a content as an additional parameter.
*/
QtScriptedEvent::QtScriptedEvent(
        const QString & name,
        const QStringList & paramNames,
        const QVariantList & paramValues,
        const QScriptValue & content)

        : QEvent(QtScriptedEvent::eventType()),ename(name),pnames(paramNames),pvals(paramValues),cnt(content)
{
    metaData.kind = MetaData::Internal;
}

/*! \class QtScriptedTransition
  \brief The QtScriptedTransition class stands for a transition that responds to QtScriptedEvent, and can be made conditional with a \l conditionExpression.
  Equivalent to the SCXML transition tag.

  \ingroup sctools
  */
/*! \property QtScriptedTransition::eventPrefix
  The event prefix to be used when testing if the transition needs to be invoked.
  Uses SCXML prefix matching. Use * to handle any event.
  */
/*! \property QtScriptedTransition::conditionExpression
  A QtScript expression that's evaluated to test whether the transition needs to be invoked.
  */

/*!
  Creates a new QtScriptedTransition from \a state, that uses \a machine to evaluate the conditions.
  */
QtScriptedTransition::QtScriptedTransition (QtState* state,QtScriptedStateMachine* machine)
    : QtTransition(state),ssm(machine)
{
}

/*!
  \internal
  */
bool QtScriptedTransition::eventTest(QEvent *e) const
{
    QScriptEngine* engine = ssm->scriptEngine();
    QString ev;

    if (e) {
        if (e->type() == QtScriptedEvent::eventType()) {
            ev = ((QtScriptedEvent*)e)->eventName();
        } else if (e->type() == QEvent::Type(QEvent::User-2)) {
            ev = QString("done.state.") + ((QtStateFinishedEvent*)e)->state()->objectName();
        }
        if (!(eventPrefix() == "*" || eventPrefix() == ev || ev.startsWith(eventPrefix()+".")))
            return false;
    }


    if (!conditionExpression().isEmpty()) {

        QScriptValue v = engine->evaluate(conditionExpression(),ssm->baseUrl().toLocalFile());
        if (engine->hasUncaughtException()) {

            qDebug() << engine->uncaughtException().toString();
            QtScriptedEvent* e = new QtScriptedEvent("error.illegalcond",
                                         QStringList()<< "error" << "expr" << "line" << "backtrace",
                                         QVariantList()
                                            << QVariant(engine->uncaughtException().toString())
                                            << QVariant(conditionExpression())
                                            << QVariant(engine->uncaughtExceptionLineNumber())
                                            << QVariant(engine->uncaughtExceptionBacktrace()));
            e->metaData.kind = QtScriptedEvent::MetaData::Platform;
            ssm->postEvent(e);
            engine->clearExceptions();
            return false;
        }
        return v.toBoolean();
    }

    return true;
}

class QtSsmDefaultInvoker : public QtSsmInvoker
{
    Q_OBJECT

    public:
    QtSsmDefaultInvoker(QtScriptedEvent* ievent, QtScriptedStateMachine* p) : QtSsmInvoker(ievent,p),childSm(0)
    {
        childSm = QtScriptedStateMachine::load (ievent->metaData.origin.resolved(ievent->metaData.target).toLocalFile(),this);
        if (childSm == NULL) {
            postParentEvent("error.targetunavailable");
        } else {
           connect(childSm,SIGNAL(finished()),this,SLOT(deleteLater()));

        }

    }

    static void initInvokerFactory(QtScriptedStateMachine*) {}

    static bool isTypeSupported(const QString & t) { return t.isEmpty() || t.toLower() == "scxml"; }

    public Q_SLOTS:
    void activate ()
    {
        if (childSm)
            childSm->start();
    }

    void cancel ()
    {
        if (childSm)
            childSm->stop();

    }

    private:
        QtScriptedStateMachine* childSm;
};
class QtSsmBindingInvoker : public QtSsmInvoker
{
    Q_OBJECT
    QScriptValue content;
    QScriptValue stored;

    public:
    QtSsmBindingInvoker(QtScriptedEvent* ievent, QtScriptedStateMachine* p) : QtSsmInvoker(ievent,p)
    {
    }

    static void initInvokerFactory(QtScriptedStateMachine*) {}

    static bool isTypeSupported(const QString & t) { return t.toLower() == "q-bindings"; }

    public Q_SLOTS:
    void activate ()
    {
        QScriptEngine* engine = ((QtScriptedStateMachine*)parent())->scriptEngine();
        QScriptValue content = initEvent->content();
        if (content.isArray()) {
            stored = content.engine()->newArray(content.property("length").toInt32());

            QScriptValueIterator it (content);
            for (int i=0; it.hasNext(); ++i) {
                it.next();
                if (it.value().isArray()) {
                    QScriptValue object = it.value().property(0);
                    QString property = it.value().property(1).toString();
                    QScriptValue val = it.value().property(2);
                    QScriptValue arr = engine->newArray(3);
                    arr.setProperty("0",it.value().property(0));
                    arr.setProperty("1",it.value().property(1));
                    if (object.isQObject()) {
                        QObject* o = object.toQObject();
                        arr.setProperty("2",engine->newVariant(o->property(property.toAscii().constData())));
                        o->setProperty(property.toAscii().constData(),val.toVariant());
                    } else if (object.isObject()) {
                        arr.setProperty("2",object.property(property));
                        object.setProperty(property,val);
                    }
                    stored.setProperty(i,arr);
                }
            }
        }
    }

    void cancel ()
    {
        if (stored.isArray()) {
            QScriptValueIterator it (stored);
            while (it.hasNext()) {
                it.next();
                if (it.value().isArray()) {
                    QScriptValue object = it.value().property(0);
                    QString property = it.value().property(1).toString();
                    QScriptValue val = it.value().property(2);
                    if (object.isQObject()) {
                        QObject* o = object.toQObject();
                        o->setProperty(property.toAscii().constData(),val.toVariant());
                    } else if (object.isObject()) {
                        object.setProperty(property,val);
                    }
                }
            }
        }
    }
};
  
/*!
\fn QtSsmInvoker::~QtSsmInvoker()
*/


/*!
\fn QtScriptedStateMachine::eventTriggered(const QString & name)

This signal is emitted when external event \a name is handled in the state machine. 
*/

/*!
  Creates a new QtScriptedStateMachine object, with parent \a parent.
  */

QtScriptedStateMachine::QtScriptedStateMachine(QObject* parent)
    : QtStateMachine(parent)
{
    pvt = new QtScriptedStateMachinePrivate;
    pvt->scriptEng = new QScriptEngine(this);
    QScriptValue glob = pvt->scriptEng->globalObject();
    QScriptValue utilObj = pvt->scriptEng->newObject();
    glob.setProperty("SMUTIL_clone",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::deepCopy));
    glob.setProperty("In",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::isInState));
    glob.setProperty("_rcvSig",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::receiveSignal));
    glob.setProperty("print",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::script_print));
    glob.setProperty("SMUTIL_postEvent",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::postEvent));
    glob.setProperty("SMUTIL_invoke",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::invoke));
    glob.setProperty("SMUTIL_cssTime",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::cssTime));
    glob.setProperty("SMUTIL_stateMachine",pvt->scriptEng->newQObject(this));
    glob.setProperty("setTimeout",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::setTimeout));
    glob.setProperty("clearTimeout",pvt->scriptEng->newFunction(QtScriptedStateMachineFunctions::clearTimeout));
    QScriptValue dmObj = pvt->scriptEng->newObject();
    glob.setProperty("_data",pvt->scriptEng->newObject());
    glob.setProperty("_global",pvt->scriptEng->globalObject());
    glob.setProperty("connectSignalToEvent",pvt->scriptEng->evaluate("function(sig,ev) {sig.connect({'e':ev},_rcvSig);}"));
    static QtSsmAutoInvokerFactory<QtSsmDefaultInvoker> _s_defaultInvokerFactory;
    static QtSsmAutoInvokerFactory<QtSsmBindingInvoker> _s_bindingInvokerFactory;
    registerInvokerFactory(&_s_defaultInvokerFactory);
    registerInvokerFactory(&_s_bindingInvokerFactory);
    connect(this,SIGNAL(started()),this,SLOT(registerSession()));
    connect(this,SIGNAL(stopped()),this,SLOT(unregisterSession()));
#ifdef QT_GUI_LIB
    static QtSsmAutoInvokerFactory<QtSsmMenuInvoker> _s_msgboxInvokerFactory;
    static QtSsmAutoInvokerFactory<QtSsmMessageBoxInvoker> _s_menuInvokerFactory;
    registerInvokerFactory(&_s_msgboxInvokerFactory);
    registerInvokerFactory(&_s_menuInvokerFactory);
#endif
}

/*! \internal */
void QtScriptedStateMachine::beginSelectTransitions(QEvent* ev)
{
    QScriptValue eventObj = pvt->scriptEng->newObject();
    if (ev) {
        if (ev->type() == QtScriptedEvent::eventType()) {
            QtScriptedEvent* se = (QtScriptedEvent*)ev;
            eventObj.setProperty("name",qScriptValueFromValue<QString>(pvt->scriptEng,se->eventName()));
            eventObj.setProperty("target",qScriptValueFromValue(pvt->scriptEng,QVariant::fromValue<QUrl>(se->metaData.target)));
            eventObj.setProperty("targettype",qScriptValueFromValue<QString>(pvt->scriptEng,se->metaData.targetType));
            eventObj.setProperty("invokeid",qScriptValueFromValue<QString>(pvt->scriptEng,se->metaData.invokeID));
            eventObj.setProperty("origin",QScriptValue(qScriptValueFromValue(pvt->scriptEng,QVariant::fromValue<QUrl>(se->metaData.origin))));
            eventObj.setProperty("originType",qScriptValueFromValue<QString>(pvt->scriptEng,se->metaData.originType));
            switch (se->metaData.kind) {
                case QtScriptedEvent::MetaData::Internal:
                    eventObj.setProperty("kind",qScriptValueFromValue<QString>(pvt->scriptEng, "internal"));
                break;
                case QtScriptedEvent::MetaData::External:
                    eventObj.setProperty("kind",qScriptValueFromValue<QString>(pvt->scriptEng, "external"));
                break;
                case QtScriptedEvent::MetaData::Platform:
                    eventObj.setProperty("kind",qScriptValueFromValue<QString>(pvt->scriptEng, "platform"));
                default:
                break;

            }

            QScriptValue dataObj = pvt->scriptEng->newObject();
            int i=0;
            foreach (QString s, se->paramNames()) {
                QScriptValue v = qScriptValueFromValue(pvt->scriptEng, se->paramValues()[i]);
                dataObj.setProperty(QString::number(i),v);
                dataObj.setProperty(s,v);
                ++i;
            }
            eventObj.setProperty("data",dataObj);
            emit eventTriggered(se->eventName());
        } else if (ev->type() == QEvent::Type(QEvent::User-2)) {
            QString n = QString("done.state.")+((QtStateFinishedEvent*)ev)->state()->objectName();
            eventObj.setProperty("name",qScriptValueFromValue<QString>(pvt->scriptEng, n));
            emit eventTriggered(n);
        }
    }
    scriptEngine()->globalObject().setProperty("_event",eventObj);

    QHash<QString,QtAbstractState*> curTargets;

    for (int i = pvt->snapshotStack.size()-1; i >= 0 && curTargets.size() < pvt->anchorTransitions.keys().size(); --i) {
        if (!curTargets.contains(pvt->snapshotStack.at(i).anchorType)) {
            curTargets[pvt->snapshotStack.at(i).anchorType] = pvt->snapshotStack.at(i).state;
        }
    }
    for (QMultiHash<QString,QtTransition*>::const_iterator it = pvt->anchorTransitions.constBegin(); it != pvt->anchorTransitions.constEnd(); ++it) {
            it.value()->setTargetState(curTargets[it.key()]);
    }

}

/*! \internal */
void QtScriptedStateMachine::endMicrostep(QEvent*)
{
    scriptEngine()->globalObject().setProperty("_event",QScriptValue());
    for (QHash<QString,QtScriptedStateMachinePrivate::AnchorSnapshot>::iterator
         it = pvt->curSnapshot.begin();
         it != pvt->curSnapshot.end(); ++it) {

        pvt->snapshotStack.push(it.value());

    }
    if (pvt->snapshotStack.size() > QtScriptedStateMachinePrivate::MaxSnapshots) {
        pvt->snapshotStack.remove(0,pvt->snapshotStack.size()-100);
    }
    pvt->curSnapshot.clear();
}

/*! Returns the script engine attached to the state-machine. */
QScriptEngine* QtScriptedStateMachine::scriptEngine () const
{
    return pvt->scriptEng;
}

/*!
    Registers object \a o to the script engine attached to the state machine.
    The object can be accessible from global variable \a name. If \a name is not provided,
    the object's name is used. If \a recursive is true, all the object's decendants are registered
    as global objects, with their respective object names as variable names.
*/
void QtScriptedStateMachine::registerObject (QObject* o, const QString & name, bool recursive)
{
    QString n(name);
    if (n.isEmpty())
        n = o->objectName();
    if (!n.isEmpty())
        pvt->scriptEng->globalObject().setProperty(n,pvt->scriptEng->newQObject(o));
    if (recursive) {
        QObjectList ol = o->findChildren<QObject*>();
        foreach (QObject* oo, ol) {
            if (!oo->objectName().isEmpty())
                registerObject(oo);
        }
    }
}

/*!
  Posts a QtScriptedEvent named \a event, with no payload.
  \sa QtScriptedEvent
  */
void QtScriptedStateMachine::postNamedEvent(const QString & event)
{
    QtScriptedEvent* e = new QtScriptedEvent(event);
    e->metaData.kind = QtScriptedEvent::MetaData::External;
    postEvent(e);
}
/*!
    Executes script \a s in the attached script engine.
    If the script fails, a "error.illegalvalue" event is posted to the state machine.
*/

void QtScriptedStateMachine::executeScript (const QString & s)
{
        pvt->scriptEng->evaluate (s,baseUrl().toLocalFile());
        if (pvt->scriptEng->hasUncaughtException()) {
            QtScriptedEvent* e = new QtScriptedEvent("error.illegalvalue",
                                         QStringList()<< "error" << "expr" << "line" << "backtrace",
                                         QVariantList()
                                            << QVariant(pvt->scriptEng->uncaughtException().toString())
                                            << QVariant(s)
                                            << QVariant(pvt->scriptEng->uncaughtExceptionLineNumber())
                                            << QVariant(pvt->scriptEng->uncaughtExceptionBacktrace()));
            e->metaData.kind = QtScriptedEvent::MetaData::Platform;
            postEvent(e);
            pvt->scriptEng->clearExceptions();
        }
}

/*!
  Enabled invoker factory \a f to be called from <invoke /> tags.
  */

void QtScriptedStateMachine::registerInvokerFactory (QtSsmInvokerFactory* f)
{
    pvt->invokerFactories << f;
	f->init(this);
}

/*! \class QtSsmInvoker
    \brief The QtSsmInvoker class an invoker, which the state-machine context can activate or cancel
        with an <invoke> tag.

    \ingroup sctools

    An invoker is a object that represents an external component that the state machine
    can activate when the encompassing state is entered, or cancel when the encompassing
    state is exited from.
  */

/*! \fn QtSsmInvoker::QtSsmInvoker(QtScriptedEvent* ievent, QtStateMachine* parent)
    When reimplementing the constructor, always use the two parameters (\a ievent and \a parent),
    as they're called from QtSsmInvokerFactory.
*/

/*! \fn  QtSsmInvoker::activate() 
    This function is called when the encompassing state is entered.
    The call to this function from the state-machine context is asynchronous, to make sure
    that the state is not exited during the same step in which it's entered.

*/

/*! \fn QtSsmInvoker::cancel()
    Reimplement this function to allow for asynchronous cancellation of the invoker.
    It's the invoker's responsibility to delete itself after this function has been called.
    The default implementation deletes the invoker.
*/

/*! \fn QtScriptedStateMachine* QtSsmInvoker::parentStateMachine()
  Returns the state machine encompassing the invoker.
  */

/*!
  Posts an event \a e to the state machine encompassing the invoker.
  */
void QtSsmInvoker::postParentEvent (QtScriptedEvent* e)
{
    e->metaData.origin = initEvent->metaData.target;
    e->metaData.target = initEvent->metaData.origin;
    e->metaData.originType = initEvent->metaData.targetType;
    e->metaData.targetType = initEvent->metaData.originType;
    e->metaData.kind = QtScriptedEvent::MetaData::External;
    e->metaData.invokeID = initEvent->metaData.invokeID;
    parentStateMachine()->postEvent(e);
}
/*! \overload
  Posts a QtScriptedEvent named \a e to the encompassing state machine.
  */
void QtSsmInvoker::postParentEvent(const QString & e)
{
    QtScriptedEvent* ev = new QtScriptedEvent(e);
    ev->metaData.kind = QtScriptedEvent::MetaData::External;
    postParentEvent(ev);
}
/*! \internal */
QtScriptedStateMachine::~QtScriptedStateMachine()
{
    delete pvt;
}

QtSsmInvoker::~QtSsmInvoker()
{
    postParentEvent("CancelResponse");
}
/*!
    \property QtScriptedStateMachine::baseUrl
    The url used to resolve scripts and invoke urls.
*/
QUrl QtScriptedStateMachine::baseUrl() const
{
    return pvt->burl;
}

void QtScriptedStateMachine::setBaseUrl(const QUrl & u)
{
    pvt->burl = u;
}

void QtScriptedStateMachine::registerSession()
{
    pvt->sessionID = QUuid::createUuid().toString();
    pvt->sessions[pvt->sessionID] = this;
    pvt->scriptEng->globalObject().setProperty("_sessionid",qScriptValueFromValue<QString>(scriptEngine(), pvt->sessionID));
    executeScript(pvt->startScript);
}

void QtScriptedStateMachine::unregisterSession()
{
    pvt->scriptEng->globalObject().setProperty("_sessionid",QScriptValue());
    pvt->sessions.remove(pvt->sessionID);
}

/*!
	Returns a statically-generated event type to be used by SCXML events.
*/
QEvent::Type QtScriptedEvent::eventType()
{
    static QEvent::Type _t = (QEvent::Type)QEvent::registerEventType(QEvent::User+200);
    return _t;
}
const char SCXML_NAMESPACE [] = "http://www.w3.org/2005/07/scxml";



struct ScTransitionInfo
{

    QtScriptedTransition* transition;
    QStringList targets;
    QString anchor;
    QtStateAction* action;
    ScTransitionInfo() : action(0) {}
};

struct ScStateInfo
{
    QString initial;
};

struct ScHistoryInfo
{
    QtHistoryState* hstate;
    QString defaultStateID;
};

struct ScExecContext
{
    QtScriptedStateMachine* sm;
    QString script;
    enum {None, StateEntry,StateExit,Transition } type;
    QtScriptedTransition* trans;
    QtActionState* state;
    ScExecContext() : sm(NULL),type(None),trans(NULL),state(NULL)
    {
    }

    void applyScript()
    {
        if (!script.isEmpty()) {
            QtStateAction* a = new QtStateInvokeMethodAction(sm,"executeScript",QVariantList()<<script);
            switch(type) {
                case StateEntry:
                    state->addEntryAction(a);
                break;
                case StateExit:
                    state->addExitAction(a);
                break;
                case Transition:
                    trans->addAction(a);
                break;
                default:
                delete a;
                break;
            }
        }
    }
};

class QtScStreamLoader
{
    public:
    QtScriptedStateMachine* stateMachine;

    QList<ScTransitionInfo> transitions;
    QHash<QtState*,ScStateInfo> stateInfo;
    QList<ScHistoryInfo> historyInfo;
    QHash<QString,QtAbstractState*> stateByID;
    QSet<QString> signalEvents;
    void loadState (QtState* state, QIODevice* dev, const QString & stateID,const QString & filename);
    QtScriptedStateMachine* load (QIODevice* device, QObject* obj = NULL, const QString & filename = "");

    QScriptValue evaluateFile (const QString & fn)
    {
        QFile f (fn);
        f.open(QIODevice::ReadOnly);
        return stateMachine->scriptEngine()->evaluate(QString::fromUtf8(f.readAll()),fn);
    }
};

class QtStateSaveAnchorAction : public QtStateAction
{
    public:
    QtScriptedStateMachine* sm;
    QtScriptedStateMachinePrivate* pvt;
    QtScriptedStateMachinePrivate::AnchorSnapshot anchorSnapshot;
    QtStateSaveAnchorAction(QtScriptedStateMachine* p,QtScriptedStateMachinePrivate* pv,
                            const QString & type, const QString & loc,
                            QtAbstractState* s) :
        QtStateAction(p),sm(p),pvt(pv)
    {
        anchorSnapshot.anchorType = type;
        anchorSnapshot.location = loc;
        anchorSnapshot.state = s;
    }

    virtual void execute ()
    {
        if (!anchorSnapshot.location.isEmpty()) {
            anchorSnapshot.snapshot = _q_deepCopy(sm->scriptEngine()->evaluate(anchorSnapshot.location));
        }
        pvt->curSnapshot[anchorSnapshot.anchorType] = anchorSnapshot;
    }
};

class QtStateRestoreAnchorAction : public QtStateAction
{
    public:
    QtScriptedStateMachine* sm;
    QtScriptedStateMachinePrivate* pvt;
    QString anchorType;
    QtStateRestoreAnchorAction(QtScriptedStateMachine* p,QtScriptedStateMachinePrivate* pv,
                            const QString & type) :
        QtStateAction(p),sm(p),pvt(pv),anchorType(type)
    {

    }

    virtual void execute ()
    {
        pvt->curSnapshot.clear();
        while (!pvt->snapshotStack.isEmpty()) {
            QtScriptedStateMachinePrivate::AnchorSnapshot s = pvt->snapshotStack.pop();
            if (s.anchorType == anchorType) {
                if (s.location != "") {
                    sm->scriptEngine()->globalObject().setProperty("_snapshot",s.snapshot);
                    sm->scriptEngine()->evaluate(QString ("%1 = _snapshot;").arg(s.location));
                    sm->scriptEngine()->globalObject().setProperty("_snapshot",QScriptValue());
                }
                break;
            }
        }
    }
};

static QString sanitize (const QString & str)
{
    return QString("eval(unescape(\"%1\"))").
            arg(QString::fromAscii(str.trimmed().toUtf8().toPercentEncoding(QByteArray("[]()<>;:#/'`_-., \t@!^&*{}"))));
}
static QString sanitize (const QStringRef & str)
{
    return sanitize(str.toString());
}

void QtScStreamLoader::loadState (
        QtState* stateParam,
        QIODevice *dev,
        const QString & stateID,
        const QString & filename)
{
    QXmlStreamReader r (dev);
    QtState* curState = NULL;
    ScExecContext curExecContext;
    curExecContext.sm = stateMachine;
    QtState* topLevelState = NULL;
    QtHistoryState* curHistoryState = NULL;
    QString initialID = "";
    QString idLocation;
    QtScriptedTransition* curTransition = NULL;
    bool inRoot = true;
    while (!r.atEnd()) {
        r.readNext();
        if (r.hasError()) {
            qDebug() << QString("SCXML read error at line %1, column %2: %3").arg(r.lineNumber()).arg(r. columnNumber()).arg(r.errorString());
            return;
        }
        if (r.namespaceUri() == SCXML_NAMESPACE || r.namespaceUri() == "") {
            if (r.isStartElement()) {
                if (r.name().toString().compare("scxml",Qt::CaseInsensitive) == 0) {
                    if (stateID == "") {
                        topLevelState = curState = stateParam;
                        stateInfo[curState].initial = r.attributes().value("initial").toString();
                        if (curState == stateMachine->rootState()) {
                            stateMachine->scriptEngine()->globalObject().setProperty("_name",qScriptValueFromValue<QString>(stateMachine->scriptEngine(),r.attributes().value("name").toString()));
                        }

                    }
                } else if (r.name().toString().compare("state",Qt::CaseInsensitive) == 0 || r.name().toString().compare("parallel",Qt::CaseInsensitive) == 0) {
                    inRoot = false;
                    QString id = r.attributes().value("id").toString();
                    QtState* newState = NULL;
                    if (curState) {
                        newState= new QtState(r.name().toString().compare("parallel",Qt::CaseInsensitive) == 0 ? QtState::ParallelGroup : QtState::Normal,
                                                        curState);
                    } else if (id == stateID) {
                        topLevelState = newState = stateParam;

                    }
                    if (newState) {
                        stateInfo[newState].initial = r.attributes().value("initial").toString();
                        newState->setObjectName(id);
                        if (!id.isEmpty() && stateInfo[curState].initial == id) {

                            if (curState == stateMachine->rootState())
                                stateMachine->setInitialState(newState);
                            else
                                curState->setInitialState(newState);
                        }
                        QString src = r.attributes().value("src").toString();
                        if (!src.isEmpty()) {
                            int refidx = src.indexOf('#');
                            QString srcfile, refid;
                            if (refidx > 0) {
                                srcfile = src.left(refidx);
                                refid = src.mid(refidx+1);
                            } else
                                srcfile = src;
                            srcfile = QDir::cleanPath( QFileInfo(filename).dir().absoluteFilePath(srcfile));
                            QFile newFile (srcfile);
                            if (newFile.exists()) {
                                newFile.open(QIODevice::ReadOnly);
                                loadState(newState,&newFile,refid,srcfile);
                            }
                        }
                        initialID = r.attributes().value("initial").toString();
                        stateByID[id] = newState;
                        curState = newState;
                        curExecContext.state = newState;
                    }

                } else if (r.name().toString().compare("initial",Qt::CaseInsensitive) == 0) {
                    if (curState && stateInfo[curState].initial == "") {
                        QtState* newState = new QtState(curState);
                        curState->setInitialState(newState);
                    }
                } else if (r.name().toString().compare("history",Qt::CaseInsensitive) == 0) {
                    if (curState) {
                        QString id = r.attributes().value("id").toString();
                        curHistoryState = curState->addHistoryState(r.attributes().value("type") == "shallow" ? QtState::ShallowHistory : QtState::DeepHistory);
                        curHistoryState->setObjectName(id);
                        stateByID[id] = curHistoryState;
                    }
                } else if (r.name().toString().compare("final",Qt::CaseInsensitive) == 0) {
                    if (curState) {
                        QString id = r.attributes().value("id").toString();
                        QtFinalState* f = new QtFinalState(curState);
                        f->setObjectName(id);
                        curExecContext.state = f;
                        stateByID[id] = f;
                    }
                } else if (r.name().toString().compare("script",Qt::CaseInsensitive) == 0) {
                    QString txt = r.readElementText().trimmed();
                    if (curExecContext.type == ScExecContext::None && curState == topLevelState) {
                        stateMachine->executeScript(txt);
                    } else
                        curExecContext.script += txt;
                } else if (r.name().toString().compare("log",Qt::CaseInsensitive) == 0) {
                    curExecContext.script +=
                            QString("print('[' + %1 + '][' + %2 + ']' + %3)")
                            .arg(sanitize(r.attributes().value("label")))
                            .arg(sanitize(r.attributes().value("level")))
                            .arg(sanitize(r.attributes().value("expr")));

                } else if (r.name().toString().compare("assign",Qt::CaseInsensitive) == 0) {
                    QString locattr = r.attributes().value("location").toString();
                    if (locattr.isEmpty()) {
                        locattr = r.attributes().value("dataid").toString();
                        if (!locattr.isEmpty())
                            locattr = "_data." + locattr;
                    }
                    if (!locattr.isEmpty()) {
                        curExecContext.script += QString ("%1 = %2;").arg(locattr).arg(sanitize(r.attributes().value("expr")));
                    }
                } else if (r.name().toString().compare("if",Qt::CaseInsensitive) == 0) {
                    curExecContext.script += QString("if (%1) {").arg(sanitize(r.attributes().value("cond")));
                } else if (r.name().toString().compare("elseif",Qt::CaseInsensitive) == 0) {
                    curExecContext.script += QString("} elseif (%1) {").arg(sanitize(r.attributes().value("cond")));
                } else if (r.name().toString().compare("else",Qt::CaseInsensitive) == 0) {
                    curExecContext.script += " } else { ";
                } else if (r.name().toString().compare("cancel",Qt::CaseInsensitive) == 0) {
                    curExecContext.script += QString("clearTimeout (%1)").arg(sanitize(r.attributes().value("id")));
                } else if (r.name().toString().compare("onentry",Qt::CaseInsensitive) == 0) {
                    curExecContext.type = ScExecContext::StateEntry;
                    curExecContext.script = "";
                } else if (r.name().toString().compare("onexit",Qt::CaseInsensitive) == 0) {
                    curExecContext.type = ScExecContext::StateExit;
                    curExecContext.script = "";
                } else if (r.name().toString().compare("raise",Qt::CaseInsensitive) == 0 || r.name().toString().compare("event",Qt::CaseInsensitive) == 0 ) {
                    QString ev = r.attributes().value("event").toString();
                    if (ev.isEmpty())
                        ev = r.attributes().value("name").toString();
                    curExecContext.script +=
                            QString("{"
                                "var paramNames = []; var paramValues = []; "
                                "var content = ''; var eventName='%1'; "
                                "var target = '_internal'; var targetType = 'scxml'; ").arg(ev);

                } else if (r.name().toString().compare("send",Qt::CaseInsensitive) == 0) {
				    QString type = r.attributes().value("type").toString();
					if (type.isEmpty())
						type = r.attributes().value("targettype").toString();
                    curExecContext.script +=
                            QString("{"
                                "var paramNames = [%1]; var paramValues = []; "
                                "var content = ''; var eventName=%2; "
                                "var targetType = %3; var target = %4;")
                            .arg(r.attributes().value("namelist").toString().replace(" ",","))
                            .arg(sanitize(r.attributes().value("event").toString()))
                            .arg(type.isEmpty() ? "'scxml'" : sanitize(r.attributes().value("type")))
                            .arg(r.attributes().value("target").length() ? sanitize(r.attributes().value("target")) : "''");
                    idLocation = r.attributes().value("idlocation").toString();
                    if (idLocation.isEmpty())
                        idLocation = r.attributes().value("sendid").toString();

                   curExecContext.script += QString("var delay = %1; ").arg(r.attributes().value("delay").length()
                             ? QString("SMUTIL_cssTime(%1)").arg(sanitize(r.attributes().value("delay")))
                             : "0");
                } else if (r.name().toString().compare("invoke",Qt::CaseInsensitive) == 0) {
                    idLocation = r.attributes().value("idlocation").toString();
                        if (idLocation.isEmpty())
                            idLocation = r.attributes().value("invokeid").toString();
                        curState->addExitAction(new QtStateInvokeMethodAction(stateMachine,"executeScript",QVariantList() <<
                                    QString("invoke_%1.cancel();").arg(curState->objectName())));

				    QString type = r.attributes().value("type").toString();
					if (type.isEmpty())
						type = r.attributes().value("targettype").toString();
                    curExecContext.type = ScExecContext::StateEntry;
                    curExecContext.state = curState;
                    curExecContext.script =
                            QString("{"
                                "var paramNames = []; var paramValues = []; "
                                "var content = ''; "
                                "var srcType = \"%1\"; var src = %2;")
                            .arg(type.length() ? type : "scxml")
                            .arg(r.attributes().value("src").length() ? sanitize(r.attributes().value("target")) : "\"\"");


                } else if (r.name().toString().compare("transition",Qt::CaseInsensitive) == 0) {
                    if (curHistoryState) {
                        ScHistoryInfo inf;
                        inf.hstate = curHistoryState;
                        inf.defaultStateID = r.attributes().value("target").toString();
                        historyInfo.append(inf);
                    } else {
                        ScTransitionInfo inf;
                        inf.targets = r.attributes().value("target").toString().split(' ');
                        curExecContext.type = ScExecContext::Transition;
                        curExecContext.script = "";
                        curTransition = new QtScriptedTransition(curState,stateMachine);
                        curTransition->setConditionExpression(r.attributes().value("cond").toString());
                        curTransition->setEventPrefix(r.attributes().value("event").toString());
                        curExecContext.trans = curTransition;
                        QString anc = r.attributes().value("anchor").toString();
                        if (!anc.isEmpty()) {
                            stateMachine->pvt->anchorTransitions.insert(anc,curTransition);
                            curTransition->addAction(new QtStateRestoreAnchorAction(stateMachine,stateMachine->pvt,anc));
                        }
                        inf.transition = curTransition;
                        transitions.append(inf);
                        if (curTransition->eventPrefix().startsWith("q-signal:")) {
                            signalEvents.insert(curTransition->eventPrefix());
                        }
                        curTransition->setObjectName(QString ("%1 to %2 on %3 if %4 (anchor=%5)").arg(curState->objectName()).arg(inf.targets.join(" ")).arg(curTransition->eventPrefix()).arg(curTransition->conditionExpression()).arg(anc));
                    }
                } else if (r.name().toString().compare("anchor",Qt::CaseInsensitive) == 0) {
                    curState->addExitAction(new QtStateSaveAnchorAction(stateMachine,stateMachine->pvt,r.attributes().value("type").toString(),r.attributes().value("snapshot").toString(),curState));
                 } else if (r.name().toString().compare("data",Qt::CaseInsensitive) == 0) {
                    QScriptValue val = qScriptValueFromValue<QString>(stateMachine->scriptEngine(),"")  ;
                    QString id = r.attributes().value("id").toString();
                    if (r.attributes().value("src").length())
                        val = evaluateFile(QFileInfo(filename).dir().absoluteFilePath(r.attributes().value("src").toString()));
                    else {
                        if (r.attributes().value("expr").length()) {
                            val = stateMachine->scriptEngine()->evaluate(r.attributes().value("expr").toString());
                        } else {
                            QString t = r.readElementText();
                            if (!t.isEmpty())
                                val = stateMachine->scriptEngine()->evaluate(t);
                        }
                    }
                    stateMachine->scriptEngine()->evaluate("_data")
                            .setProperty(id,val);
                } else if (r.name().toString().compare("param",Qt::CaseInsensitive) == 0) {
                    curExecContext.script +=
                            QString("paramNames[paramNames.length] = \"%1\";")
                                .arg(r.attributes().value("name").toString());
                    curExecContext.script +=
                            QString("paramValues[paramValues.length] = %1;")
                                .arg(sanitize(r.attributes().value("expr")));

                } else if (r.name().toString().compare("content",Qt::CaseInsensitive) == 0) {
                    curExecContext.script += QString("content = %1; ").arg(sanitize(r.readElementText()));
                }
        } else if (r.isEndElement()) {
             if (r.name().toString().compare("state",Qt::CaseInsensitive) == 0 || r.name().toString().compare("parallel",Qt::CaseInsensitive) == 0) {
                 if (curState == topLevelState) {
                     return;
                 } else {
                     curState = qobject_cast<QtState*>(curState->parent());
                     curExecContext.state = curState;
                 }
             } else if (r.name().toString().compare("history",Qt::CaseInsensitive) == 0) {
                 curHistoryState = NULL;
             } else if (r.name().toString().compare("final",Qt::CaseInsensitive) == 0) {
                 curExecContext.state = (QtActionState*)curExecContext.state->parent();
            } else if (r.name().toString().compare("send",Qt::CaseInsensitive) == 0) {
                if (!idLocation.isEmpty())
                    curExecContext.script += idLocation + " = ";
                    curExecContext.script += QString("setTimeout(function() { "
                                "SMUTIL_postEvent("
                                 "eventName,target,targetType,paramNames,paramValues,content"
                                 ");"
                            "}, delay); }");
               idLocation = "";
            } else if (r.name().toString().compare("raise",Qt::CaseInsensitive) == 0) {
                curExecContext.script +=  "SMUTIL_postEvent(eventName,target,targetType,paramNames,paramValues,content); }";
            } else if (
                    r.name().toString().compare("onentry",Qt::CaseInsensitive) == 0
                    || r.name().toString().compare("onexit",Qt::CaseInsensitive) == 0
                    || r.name().toString().compare("scxml",Qt::CaseInsensitive) == 0) {
                curExecContext.state = curState;
                curExecContext.type = r.name().toString().compare("onexit",Qt::CaseInsensitive)==0 ? ScExecContext::StateExit : ScExecContext::StateEntry;
                curExecContext.applyScript();
                curExecContext.type = ScExecContext::None;
            } else if (r.name().toString().compare("transition",Qt::CaseInsensitive) == 0) {
                if (!curHistoryState) {
                    curExecContext.trans = curTransition;
                    curExecContext.type = ScExecContext::Transition;
                    curExecContext.applyScript();
                }

                ScTransitionInfo* ti = &(transitions.last());
                if (!curExecContext.script.isEmpty() && ti->anchor != "")
                    ti->action = new QtStateInvokeMethodAction(stateMachine,"executeScript",QVariantList() << curExecContext.script);
                curExecContext.type = ScExecContext::None;
            } else if (r.name().toString().compare("invoke",Qt::CaseInsensitive) == 0) {
                curExecContext.script +=  QString("invoke_%1 = SMUTIL_invoke(srcType,src,paramNames,paramValues,content); }").arg(curState->objectName());
                if (!idLocation.isEmpty()) {
                    curExecContext.script +=  QString("%1 = invoke_%2;").arg(idLocation).arg(curState->objectName());
                }
                curExecContext.state = curState;
                curExecContext.type = ScExecContext::StateEntry;
                curExecContext.applyScript();
                idLocation = "";
                curExecContext.type = ScExecContext::None;
            }
        }
    }
    }
}


QtScriptedStateMachine* QtScStreamLoader::load(QIODevice* device, QObject* obj, const QString & filename)
{
    stateMachine = new QtScriptedStateMachine(obj);
    // traverse through the states
    loadState(stateMachine->rootState(),device,"",filename);

    // resolve history default state
    foreach (ScHistoryInfo h, historyInfo) {
        h.hstate->setDefaultState(stateByID[h.defaultStateID]);
    }
    foreach (QString s, signalEvents) {
        QString sig = s;
        sig = sig.mid(sig.indexOf(':')+1);
        sig = sig.left(sig.indexOf('('));
        QString scr = QString("%1.connect({e:\"%2\"},_rcvSig);\n").arg(sig).arg(s);
        stateMachine->pvt->startScript += scr;
    }

    // resolve transitions

    foreach (ScTransitionInfo t, transitions) {
        QList<QtAbstractState*> states;
        if (!t.targets.isEmpty()) {
            foreach (QString s, t.targets) {
                if (!s.trimmed().isEmpty()) {
                    QtAbstractState* st = stateByID[s];
                    if (st)
                        states.append(st);
                }
            }
            t.transition->setTargetStates(states);
        }
    }

    return stateMachine;
}

/*!
    Loads a state machine from an scxml file located at \a filename, with parent object \a o.
  */
QtScriptedStateMachine* QtScriptedStateMachine::load (const QString & filename, QObject* o)
{
    QtScStreamLoader l;
    QFile f (filename);
    f.open(QIODevice::ReadOnly);
    return l.load(&f,o,filename);
}

#include "qscriptedstatemachine.moc"
