/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef RK4ADJUSTPARAMBOX_H
#define RK4ADJUSTPARAMBOX_H

#include "equares_core.h"

class EQUARES_CORESHARED_EXPORT Rk4AdjustParamBox : public Box
{
    Q_OBJECT
public:
    explicit Rk4AdjustParamBox(QObject *parent = 0);

    InputPorts inputPorts() const;
    OutputPorts outputPorts() const;
    void checkPortFormat() const;
    bool propagatePortFormat();

    RuntimeBox *newRuntimeBox() const;

private:
    mutable InputPort m_step;
    mutable InputPort m_stepCount;
    mutable InputPort m_duration;
    mutable InputPort m_outputControl;
    mutable OutputPort m_rk4param;
};

class EQUARES_CORESHARED_EXPORT Rk4AdjustParamRuntimeBox : public RuntimeBox
{
public:
    explicit Rk4AdjustParamRuntimeBox(const Rk4AdjustParamBox *box);

private:
    RuntimeInputPort m_step;
    RuntimeInputPort m_stepCount;
    RuntimeInputPort m_duration;
    RuntimeInputPort m_outputControl;
    RuntimeOutputPort m_rk4param;
    double m_data[3];

    bool processInput(int);
};

#endif // RK4ADJUSTPARAMBOX_H
