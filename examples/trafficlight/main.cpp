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
#include <qstate.h>
#include <qstatemachine.h>
#include <qfinalstate.h>
#endif

//! [0]
class LightWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool on READ isOn WRITE setOn)
public:
    LightWidget(const QColor &color, QWidget *parent = 0)
        : QWidget(parent), m_color(color), m_on(false) {}

    bool isOn() const
        { return m_on; }
    void setOn(bool on)
    {
        if (on == m_on)
            return;
        m_on = on;
        update();
    }

public slots:
    void turnOff() { setOn(false); }
    void turnOn() { setOn(true); }

protected:
    virtual void paintEvent(QPaintEvent *)
    {
        if (!m_on)
            return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(m_color);
        painter.drawEllipse(0, 0, width(), height());
    }

private:
    QColor m_color;
    bool m_on;
};
//! [0]

//! [1]
class LightState : public QtState
{
public:
    LightState(LightWidget *light, int duration, QtState *parent = 0)
        : QtState(parent)
    {
        QTimer *timer = new QTimer(this);
        timer->setInterval(duration);
        timer->setSingleShot(true);
        QtState *timing = new QtState(this);
        timing->invokeMethodOnEntry(light, "turnOn");
        timing->invokeMethodOnEntry(timer, "start");
        timing->invokeMethodOnExit(light, "turnOff");
        QtFinalState *done = new QtFinalState(this);
        timing->addTransition(timer, SIGNAL(timeout()), done);
        setInitialState(timing);
    }
};
//! [1]

//! [2]
class TrafficLightWidget : public QWidget
{
public:
    TrafficLightWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        m_red = new LightWidget(Qt::red);
        vbox->addWidget(m_red);
        m_yellow = new LightWidget(Qt::yellow);
        vbox->addWidget(m_yellow);
        m_green = new LightWidget(Qt::green);
        vbox->addWidget(m_green);
        QPalette pal = palette();
        pal.setColor(QPalette::Background, Qt::black);
        setPalette(pal);
        setAutoFillBackground(true);
    }

    LightWidget *redLight() const
        { return m_red; }
    LightWidget *yellowLight() const
        { return m_yellow; }
    LightWidget *greenLight() const
        { return m_green; }

private:
    LightWidget *m_red;
    LightWidget *m_yellow;
    LightWidget *m_green;
};
//! [2]

//! [3]
class TrafficLight : public QWidget
{
public:
    TrafficLight(QWidget *parent = 0)
        : QWidget(parent)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        TrafficLightWidget *widget = new TrafficLightWidget();
        vbox->addWidget(widget);

        QtStateMachine *machine = new QtStateMachine(this);
        LightState *redGoingYellow = new LightState(widget->redLight(), 3000);
        redGoingYellow->setObjectName("redGoingYellow");
        LightState *yellowGoingGreen = new LightState(widget->yellowLight(), 1000);
        yellowGoingGreen->setObjectName("yellowGoingGreen");
        redGoingYellow->addFinishedTransition(yellowGoingGreen);
        LightState *greenGoingYellow = new LightState(widget->greenLight(), 3000);
        greenGoingYellow->setObjectName("greenGoingYellow");
        yellowGoingGreen->addFinishedTransition(greenGoingYellow);
        LightState *yellowGoingRed = new LightState(widget->yellowLight(), 1000);
        yellowGoingRed->setObjectName("yellowGoingRed");
        greenGoingYellow->addFinishedTransition(yellowGoingRed);
        yellowGoingRed->addFinishedTransition(redGoingYellow);

        machine->addState(redGoingYellow);
        machine->addState(yellowGoingGreen);
        machine->addState(greenGoingYellow);
        machine->addState(yellowGoingRed);
        machine->setInitialState(redGoingYellow);
        machine->start();
    }
};
//! [3]

//! [4]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    TrafficLight widget;
    widget.resize(120, 300);
    widget.show();

    return app.exec();
}
//! [4]

#include "main.moc"
