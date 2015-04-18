/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "SimVisualizerWidget.h"
#include <QPainter>
#include <cmath>

inline QString linkKey(const GuiLinkTarget& t1, const GuiLinkTarget& t2)
{
    return t1.toString() + "-" + t2.toString();
}

SimVisualizerWidget::SimVisualizerWidget(QWidget *parent) :
    QWidget(parent),
    m_simVis(new SimVisualizer(this))
{
    connect(m_simVis, SIGNAL(update()), SLOT(update()));
}

SimVisualizer *SimVisualizerWidget::simVisualizer() const
{
    return m_simVis;
}

void SimVisualizerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    painter.setRenderHint(QPainter::Antialiasing);
    m_simVis->paint(&painter);
}
