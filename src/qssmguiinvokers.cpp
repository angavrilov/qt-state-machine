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
#include "qssmguiinvokers_p.h"
#include <QMenu>
#include <QDebug>
#include <QMessageBox>
#include <QScriptValueIterator>
#include <QScriptEngine>
#include <QSignalMapper>
/*

  { "parent" : parentObject,
    "trackHovers" : true/false
        "children": {{"type": "action", "text": "",},
                     {"type": "menu"},
                     {"type": "separator"} },
  */
namespace
{
    void traverseMenu (QMenu* menu, QScriptValue value, QSignalMapper* clickMap, QSignalMapper* hoverMap, bool trackHover)
    {
        QScriptValueIterator it (value);
        while (it.hasNext()) {
            it.next();
            if (it.name() == "trackHover") {
                trackHover = it.value().toBoolean();
            } else if (it.name() == "parent") {
            } else if (it.name() == "children") {
                QScriptValueIterator cit (it.value());
                while (cit.hasNext()) {
                    cit.next();
                    QString type = cit.value().property("type").toString();
                    if (type == "action") {
                        QAction* act = menu->addAction("");
                        QScriptValueIterator ait (cit.value());
                        while (ait.hasNext()) {
                            ait.next();
                            if (ait.name() != "type") {
                                act->setProperty(ait.name().toAscii().constData(),ait.value().toVariant());
                            }
                        }
                        QObject::connect(act,SIGNAL(triggered()),clickMap,SLOT(map()));
                        clickMap->setMapping(act,QString("menu.action." + cit.value().property("id").toString()));
                        if (trackHover) {
                            QObject::connect(act,SIGNAL(hovered()),hoverMap,SLOT(map()));
                            hoverMap->setMapping(act,QString("menu.hover." + cit.value().property("id").toString()));
                        }
                    } else if (type == "menu") {
                        traverseMenu(menu->addMenu(""),it.value(),clickMap,hoverMap,trackHover);
                    } else if (type == "separator") {
                        menu->addSeparator();
                    }
                }
            } else {
                menu->setProperty(it.name().toAscii().constData(),it.value().toVariant());
            }
        }
    }
};

void QtSsmMenuInvoker::activate ()
{
    QtScriptedEvent* ie = initEvent;
    QScriptValue v = ie->content();
    QWidget* parent = qobject_cast<QWidget*>(v.property("parent").toQObject());
    menu = new QMenu(parent);
    QSignalMapper* clickMap = new QSignalMapper(this);
    QSignalMapper* hoverMap = new QSignalMapper(this);
    connect (clickMap,SIGNAL(mapped(QString)), this, SLOT(postParentEvent(QString)));
    connect (hoverMap,SIGNAL(mapped(QString)), this, SLOT(postParentEvent(QString)));
    traverseMenu(menu,v,clickMap,hoverMap,false);
    menu->setParent(parent,Qt::Widget);
    menu->move(QPoint(0,0));
    menu->resize(parent->size());
    menu->show();
}
void QtSsmMenuInvoker::cancel ()
{
    if (menu)
        menu->deleteLater();
    QtSsmInvoker::cancel();
}

Q_SCRIPT_DECLARE_QMETAOBJECT(QMenu,QWidget*)
Q_SCRIPT_DECLARE_QMETAOBJECT(QMessageBox,QWidget*)
 void QtSsmMenuInvoker::initInvokerFactory(QtScriptedStateMachine* sm)
 {
     QScriptEngine* se = sm->scriptEngine();
    se->globalObject().setProperty("QMenu",qScriptValueFromQMetaObject<QMenu>(se));
 }
 void QtSsmMessageBoxInvoker::initInvokerFactory(QtScriptedStateMachine* sm)
 {
     QScriptEngine* se = sm->scriptEngine();
    se->globalObject().setProperty("QMessageBox",qScriptValueFromQMetaObject<QMessageBox>(se));
 }

void QtSsmMessageBoxInvoker::onFinished(int n) {
    postParentEvent(new QtScriptedEvent("q-messagebox.finished",QStringList()<<"result",QVariantList()<<QVariant(n)));
}
/*
    { "parent": someWidget, "buttons": ...}
  */
void QtSsmMessageBoxInvoker::activate()
{
    QScriptValue v = initEvent->content();
    QWidget* parent = qobject_cast<QWidget*>(v.property("parent").toQObject());
    messageBox = new QMessageBox(parent);
    messageBox->setModal(false);
    QScriptValueIterator it (v);
    while (it.hasNext()) {
        it.next();
        if (it.name() == "standardButtons") {
            messageBox->setStandardButtons((QMessageBox::StandardButtons)it.value().toInt32());
        } else if (it.name() == "icon") {
            messageBox->setIcon((QMessageBox::Icon)it.value().toInt32());
        } else if (it.name() != "parent") {
            messageBox->setProperty(it.name().toAscii().constData(),it.value().toVariant());
        }
    }
    connect(messageBox,SIGNAL(finished(int)),this,SLOT(onFinished(int)));
    messageBox->show ();
}

void QtSsmMessageBoxInvoker::cancel()
{
    messageBox->deleteLater();
    QtSsmInvoker::cancel();
}
