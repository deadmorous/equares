/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "SimVisualizer.h"
#include <QPainter>
#include <cmath>

inline QString linkKey(const GuiLinkTarget& t1, const GuiLinkTarget& t2)
{
    return t1.toString() + "-" + t2.toString();
}

SimVisualizer::SimVisualizer(QWidget *parent) :
    QWidget(parent),
    m_sim(0)
{
}

const GuiSimulation *SimVisualizer::simulation() const
{
    return m_sim;
}

void SimVisualizer::setSimulation(const GuiSimulation *sim)
{
    m_sim = sim;
    update();
}

void SimVisualizer::setBoxHighlight(const QString& boxName, double amount, bool status)
{
    m_boxHighlight[boxName] = HL(amount, status);
}

void SimVisualizer::setPortHighlight(const GuiLinkTarget& target, double amount, bool status)
{
    m_portHighlight[target.toString()] = HL(amount, status);
}

void SimVisualizer::setLinkHighlight(const GuiLinkTarget& from, const GuiLinkTarget& to, double amount, bool status)
{
    m_linkHighlight[linkKey(from, to)] = HL(amount, status);
}

static QSizeF textSize(QPainter *painter, const QString& text)
{
    const double LargeSize = 10000;
    return painter->boundingRect(QRectF(0, 0, LargeSize, LargeSize), text).size();
}

static QRectF boxRect(QPainter *painter, const GuiBox& box)
{
    const int HMargin = 10, VMargin = 6;
    QRectF rc(box.pos, textSize(painter, box.name));
    rc.adjust(-HMargin, -VMargin, HMargin, VMargin);
    return rc;
}

static QPointF portPos(const QRectF& rc, double pos)
{
    double x1, x2, y1, y2, t;
    if (pos < 1) {
        x1 = x2 = rc.right();
        y1 = rc.bottom();
        y2 = rc.top();
        t = pos;
    }
    else if (pos < 2) {
        x1 = rc.right();
        x2 = rc.left();
        y1 = y2 = rc.top();
        t = pos - 1;
    }
    else if (pos < 3) {
        x1 = x2 = rc.left();
        y1 = rc.top();
        y2 = rc.bottom();
        t = pos - 2;
    }
    else {
        x1 = rc.left();
        x2 = rc.right();
        y1 = y2 = rc.bottom();
        t = pos - 3;
    }
    return QPointF(x1 + t*(x2-x1), y1+t*(y2-y1));
}

static QPointF portPos(QPainter *painter, const GuiLinkTarget& linkTarget, const GuiSimulation *sim)
{
    GuiBoxes::ConstIterator itb = sim->boxes.find(linkTarget.boxName);
    Q_ASSERT(itb != sim->boxes.end());
    const GuiBox& box = *itb;
    QRectF rc = boxRect(painter, box);
    GuiPorts::const_iterator itp = box.ports.find(linkTarget.portName);
    Q_ASSERT(itp != box.ports.end());
    const GuiPort& port = *itp;
    return portPos(rc, port.pos);
}

static void cropLinkLine(QPointF& p1, QPointF& p2, double radius)
{
    QPointF d = p2 - p1;
    double L = sqrt(d.x()*d.x() + d.y()*d.y());
    if (L == 0)
        return;
    d *= radius/L;
    p1 += d;
    p2 -= d;
}

static QLineF linePart(const QLineF& line, double from, double to)
{
    QPointF p1 = line.p1(),   p2 = line.p2(),   d = p2 - p1;
    p1 += from*d;
    p2 -= (1-to)*d;
    return QLineF(p1, p2);
}

QColor SimVisualizer::hlColor(const QColor &normal, const QColor &good, const QColor &bad, const HL& hl)
{
    const QColor& mixin = hl.status == 0? bad: good;
    double rgba1[4],   rgba2[4];
    normal.getRgbF(&rgba1[0], &rgba1[1], &rgba1[2], &rgba1[3]);
    mixin.getRgbF(&rgba2[0], &rgba2[1], &rgba2[2], &rgba2[3]);
    for (int i=0; i<4; ++i)
        rgba1[i] = rgba1[i] + hl.amount*(rgba2[i]-rgba1[i]);
    return QColor::fromRgbF(rgba1[0], rgba1[1], rgba1[2], rgba1[3]);
}

QBrush SimVisualizer::hlBrush(const QBrush& normal, const QBrush&  good, const QBrush&  bad, const HL& hl)
{
    return QBrush(hlColor(normal.color(), good.color(), bad.color(), hl));
}

void SimVisualizer::setBrush(QPainter *painter, const QBrush& normal, const QBrush&  good, const QBrush&  bad, const HLMap& hlm, const QString& key)
{
    HLMap::const_iterator it = hlm.find(key);
    if (it == hlm.end())
        painter->setBrush(normal);
    else
        painter->setBrush(hlBrush(normal, good, bad, it.value()));
}

void SimVisualizer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    painter.setRenderHint(QPainter::Antialiasing);
    if (!m_sim)
        return;
    QPen boxOutlinePen(QColor(0xcccccc), 2), textPen(Qt::black), inputPortPen(QColor(0xff0000), 1), outputPortPen(QColor(0x0000ff), 1);
    QBrush boxBrush(0xeeeeee), boxBrush0(0xff6666), boxBrush1(0x66ff66),
           inputPortBrush(0xffbbbb), inputPortBrush0(0xff0000), inputPortBrush1(0x00ff00),
           outputPortBrush(0xbbbbff), outputPortBrush0(0xff0000), outputPortBrush1(0x00ff00);
    QFont f;
    const int FontSize = 16;
    const double PortRadius = 4;
    f.setPixelSize(FontSize);
    painter.setFont(f);
    foreach (const GuiBox& box, m_sim->boxes) {
        QRectF rc = boxRect(&painter, box);
        painter.setPen(boxOutlinePen);
        setBrush(&painter, boxBrush, boxBrush1, boxBrush0, m_boxHighlight, box.name);
        painter.drawRoundedRect(rc, 3, 3);
        painter.setPen(textPen);
        painter.drawText(rc, Qt::AlignCenter, box.name);
        foreach(const GuiPort& port, box.ports)
        {
            QPointF pos = portPos(rc, port.pos);
            QString targetName = GuiLinkTarget(box.name, port.name).toString();
            switch (port.dir) {
            case GuiInputPort:
                painter.setPen(inputPortPen);
                setBrush(&painter, inputPortBrush, inputPortBrush1, inputPortBrush0, m_portHighlight, targetName);
                painter.setBrush(inputPortBrush);
                break;
            case GuiOutputPort:
                painter.setPen(outputPortPen);
                setBrush(&painter, outputPortBrush, outputPortBrush1, outputPortBrush0, m_portHighlight, targetName);
                break;
            default:
                Q_ASSERT(false);
            }
            painter.drawEllipse(pos, PortRadius, PortRadius);
        }
    }

    QPen
        linkPen(QColor(0xbbbbbb), 5, Qt::SolidLine, Qt::FlatCap),
        linkPen0(QColor(0xffbbbb), 5, Qt::SolidLine, Qt::FlatCap),
        linkPen1(QColor(0xbbffbb), 5, Qt::SolidLine, Qt::FlatCap);
    foreach (const GuiLink& link, m_sim->links)
    {
        QPointF p1 = portPos(&painter, link.first, m_sim),   p2 = portPos(&painter, link.second, m_sim);
        cropLinkLine(p1, p2, PortRadius);
        bool inv = false;
        HLMap::const_iterator it = m_linkHighlight.find(linkKey(link.first, link.second));
        if (it == m_linkHighlight.end()) {
            it = m_linkHighlight.find(linkKey(link.second, link.first));
            if (it == m_linkHighlight.end()) {
                painter.setPen(linkPen);
                painter.drawLine(p1, p2);
                continue;
            }
            inv = true;
        }
        else
            inv = false;
        if (inv)
            qSwap(p1, p2);
        QLineF line(p1, p2);
        const HL& hl = it.value();
        painter.setPen(hl.status == 0? linkPen0: linkPen1);
        painter.drawLine(linePart(line, 0, hl.amount));
        painter.setPen(linkPen);
        painter.drawLine(linePart(line, hl.amount, 1));
    }

    // painter.drawText(rect(), Qt::AlignCenter, "Hello");
}
