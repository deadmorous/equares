/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "Animator.h"
#include <QTimerEvent>

Animator::Animator(QObject *parent) :
    QObject(parent),
    m_stepSubdivision(5),
    m_timerInterval(40),
    m_timerId(-1),
    m_timeStep(0)
{
}

void Animator::clear()
{
    stop();
    m_timeLine.clear();
    m_activationData.clear();
    m_timeStep = 0;
    m_subStep = 0;
}

void Animator::beginActivation(const ActivationData& d)
{
    Q_ASSERT(d.id > 0);
    m_activationData[d.id] = d;
    m_timeLine << d.id;
}

void Animator::endActivation(int id, int result)
{
    Q_ASSERT(id > 0);
    Q_ASSERT(m_activationData.contains(id));
    m_activationData[id].result = result;
    m_timeLine << -id;
}

int Animator::timeStepCount() const {
    return m_timeLine.size();
}

int Animator::timeStep() const {
    return m_timeStep;
}

int Animator::stepSubdivision() const {
    return m_stepSubdivision;
}

int Animator::timerInterval() const {
    return m_timerInterval;
}

bool Animator::isPlaying() const {
    return m_timerId != -1;
}

bool Animator::run(int maxFrameCount) {
    if (isPlaying())
        return false;
    m_subStep = 0;
    emit startAnimation();
    int frame = 0;
    while(m_timeStep < m_timeLine.size()   &&   frame < maxFrameCount) {
        nextAnimationFrame();
        ++frame;
    }
    emit endAnimation();
    return true;
}

void Animator::play()
{
    if (isPlaying())
        return;
    m_timerId = startTimer(m_timerInterval);
    Q_ASSERT(m_timerId != -1);
    m_subStep = 0;
    emit startAnimation();
}

void Animator::stop()
{
    if (!isPlaying())
        return;
    killTimer(m_timerId);
    m_timerId = -1;
    m_subStep = 0;
    emit endAnimation();
}

void Animator::setTimeStep(int timeStep) {
    m_timeStep = timeStep;
}

void Animator::setStepSubdivision(int n) {
    m_stepSubdivision = n;
}

void Animator::setTimerInterval(int msec) {
    m_timerInterval = msec;
}

void Animator::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_timerId)
        return;
    nextAnimationFrame();
}

void Animator::nextAnimationFrame()
{
    if (m_subStep >= m_stepSubdivision) {
        m_subStep = 0;
        ++m_timeStep;
    }
    else
        ++m_subStep;
    if (m_timeStep >= m_timeLine.size()) {
        stop();
        return;
    }
    int id = m_timeLine[m_timeStep];
    bool returning = id < 0;
    id = abs(id);
    ActivationData d = m_activationData.value(id);
    ActivationData cd;
    if (d.callerId) {
        Q_ASSERT(m_activationData.contains(d.callerId));
        cd = m_activationData.value(d.callerId);
    }
    double amount = static_cast<double>(m_subStep) / m_stepSubdivision;
    bool status = true;
    if (returning) {
        amount = 1 - amount;
        status = d.result;
    }
    switch (d.activatorType) {
    case InputPortActivator:
        emit setPortHighlight(d.target, amount,  status);
        if (d.handled)
            emit setBoxHighlight(d.target.boxName, amount, status);
        if (d.callerId)
            emit setLinkHighlight(cd.target, d.target, amount, status);
        break;
    case OutputPortActivator:
        emit setBoxHighlight(d.target.boxName, amount, status);
        emit setPortHighlight(d.target, amount,  status);
        break;
    case GeneratorActivator:
    case PreprocessorActivator:
    case PostprocessorActivator:
        emit setBoxHighlight(d.target.boxName, amount, status);
        break;
    default:
        Q_ASSERT(false);
    }
    emit commitAnimationFrame();
}
