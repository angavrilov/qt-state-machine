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

#include "calc.h"
#include "ui_calc.h"
#include <QSignalMapper>
CalcWidget::CalcWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    QSignalMapper* mapper = new QSignalMapper(this);
    connect (mapper, SIGNAL(mapped(QString)), this, SIGNAL(command(QString)));
    QList<QAbstractButton*> buttons = findChildren<QAbstractButton*>();
    foreach (QAbstractButton* b, buttons) {
        connect (b, SIGNAL(clicked()), mapper, SLOT(map()));
    }
    mapper->setMapping(button0,"DIGIT.0");
    mapper->setMapping(button1,"DIGIT.1");
    mapper->setMapping(button2,"DIGIT.2");
    mapper->setMapping(button3,"DIGIT.3");
    mapper->setMapping(button4,"DIGIT.4");
    mapper->setMapping(button5,"DIGIT.5");
    mapper->setMapping(button6,"DIGIT.6");
    mapper->setMapping(button7,"DIGIT.7");
    mapper->setMapping(button8,"DIGIT.8");
    mapper->setMapping(button9,"DIGIT.9");
    mapper->setMapping(buttonEq,"EQUALS");
    mapper->setMapping(buttonCE,"CE");
    mapper->setMapping(buttonC,"C");
    mapper->setMapping(buttonPoint,"POINT");
    mapper->setMapping(buttonPlus,"OPER.PLUS");
    mapper->setMapping(buttonStar,"OPER.STAR");
    mapper->setMapping(buttonMinus,"OPER.MINUS");
    mapper->setMapping(buttonDiv,"OPER.DIV");
}
CalcWidget::~CalcWidget()
{
}
