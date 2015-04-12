/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <QString>
#include <QMap>
#include <QPair>
#include <QPointF>
#include <QVector>

enum GuiPortDir { GuiInputPort, GuiOutputPort };

struct GuiPort {
    QString name;
    GuiPortDir dir;
    double pos;
    GuiPort() {}
    GuiPort(const QString& name, GuiPortDir dir, double pos = 0) : name(name), dir(dir), pos(pos) {}
};

typedef QMap<QString, GuiPort> GuiPorts;

struct GuiBox {
    QString type;
    QString name;
    GuiPorts ports;
    QPointF pos;
    GuiBox() {}
    GuiBox(const QString& type, const QString& name, const GuiPorts& ports = GuiPorts()) : type(type), name(name), ports(ports) {}
};

typedef QMap<QString, GuiBox> GuiBoxes;

struct GuiLinkTarget
{
    QString boxName;
    QString portName;
    GuiLinkTarget() {}
    explicit GuiLinkTarget(const QString& boxName, const QString& portName = QString()) : boxName(boxName), portName(portName) {}
    QString toString() const {
        return boxName + ":" + portName;
    }
};

typedef QPair<GuiLinkTarget, GuiLinkTarget> GuiLink;
typedef QVector<GuiLink> GuiLinks;

struct GuiSimulation {
    GuiBoxes boxes;
    GuiLinks links;
};


enum ActivatorType {
    InvalidActivator,
    InputPortActivator,
    OutputPortActivator,
    GeneratorActivator,
    PreprocessorActivator,
    PostprocessorActivator
};

struct ActivationData {
    int id;
    ActivatorType activatorType;
    GuiLinkTarget target;
    int callerId;
    bool handled;
    bool result;
    ActivationData() : id(0), activatorType(InvalidActivator), callerId(0), handled(false), result(false) {}
    ActivationData(int id, ActivatorType activatorType, const GuiLinkTarget& target, int callerId = 0, bool handled = true) :
        id(id), activatorType(activatorType), target(target), callerId(callerId), handled(handled), result(false) {}
};

#endif // SIM_TYPES_H
