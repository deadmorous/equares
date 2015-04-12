/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "sim_types.h"
#include <QObject>

class Animator : public QObject
{
Q_OBJECT
public:
    explicit Animator(QObject *parent = 0);
    void clear();
    void beginActivation(const ActivationData& d);
    void endActivation(int id, int result);

    int timeStepCount() const;
    int timeStep() const;
    int stepSubdivision() const;
    int timerInterval() const;
    bool isPlaying() const;

public slots:
    void play();
    void stop();
    void setTimeStep(int timeStep);
    void setStepSubdivision(int n);
    void setTimerInterval(int msec);

signals:
    void setBoxHighlight(const QString& boxName, double amount, bool status);
    void setPortHighlight(const GuiLinkTarget& target, double amount, bool status);
    void setLinkHighlight(const GuiLinkTarget& from, const GuiLinkTarget& to, double amount, bool status);
    void commitAnimationFrame();
    void startAnimation();
    void endAnimation();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QVector<int> m_timeLine; // index = time step, value = +/- activation id; + means start, - means end
    QMap<int, ActivationData> m_activationData;
    int m_stepSubdivision;
    int m_timerInterval;
    int m_timerId;
    int m_timeStep;
    int m_subStep;
    void nextAnimationFrame();
};

#endif // ANIMATOR_H
