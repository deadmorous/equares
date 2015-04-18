/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef SIMVISUALIZERWIDGET_H
#define SIMVISUALIZERWIDGET_H

#include <QWidget>
#include "SimVisualizer.h"

class SimVisualizerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimVisualizerWidget(QWidget *parent = 0);
    SimVisualizer *simVisualizer() const;
    
protected:
    void paintEvent(QPaintEvent *event);

private:
    SimVisualizer *m_simVis;
};

#endif // SIMVISUALIZERWIDGET_H
