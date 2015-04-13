/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef SIMVISUALIZER_H
#define SIMVISUALIZER_H

#include <QWidget>
#include "sim_types.h"

class SimVisualizer : public QWidget
{
    Q_OBJECT
public:
    explicit SimVisualizer(QWidget *parent = 0);
    
signals:
    
public slots:
    const GuiSimulation *simulation() const;
    void setSimulation(const GuiSimulation *sim);

    void setBoxHighlight(const QString& boxName, double amount, bool status);
    void setPortHighlight(const GuiLinkTarget& target, double amount, bool status);
    void setLinkHighlight(const GuiLinkTarget& from, const GuiLinkTarget& to, double amount, bool status);
    void startAnimation();
    void endAnimation();

protected:
    void paintEvent(QPaintEvent *event);

private:
    const GuiSimulation *m_sim;
    struct HL {
        double amount;
        int status;
        HL() : amount(0), status(1) {}
        HL(double amount, int status) : amount(amount), status(status) {}
    };

    typedef QMap<QString, HL> HLMap;
    HLMap m_boxHighlight;
    HLMap m_portHighlight;
    HLMap m_linkHighlight;
    void clearHighlights();
    static QColor hlColor(const QColor& normal, const QColor& good, const QColor& bad, const HL& hl);
    static QBrush hlBrush(const QBrush& normal, const QBrush&  good, const QBrush&  bad, const HL& hl);
    static void setBrush(QPainter *painter, const QBrush& normal, const QBrush&  good, const QBrush&  bad, const HLMap& hlm, const QString& key);
    static double portRadius(double rnormal, double rmax, const HLMap& hlm, const QString& key);
};

#endif // SIMVISUALIZER_H
