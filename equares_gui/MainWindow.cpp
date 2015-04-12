/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->action_Quit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->action_Open, SIGNAL(triggered()), SLOT(openFile()));

    m_simVis = new SimVisualizer();
    m_simVis->setSimulation(&m_sim);
    setCentralWidget(m_simVis);

    m_animator = new Animator(this);
    connect(ui->actionOpen_animation, SIGNAL(triggered()), SLOT(openAnimation()));
    connect(ui->action_Start, SIGNAL(triggered()), m_animator, SLOT(play()));
    connect(ui->actionStop, SIGNAL(triggered()), m_animator, SLOT(stop()));
    connect(m_animator, SIGNAL(setBoxHighlight(QString,double,bool)), m_simVis, SLOT(setBoxHighlight(QString,double,bool)));
    connect(m_animator, SIGNAL(setPortHighlight(GuiLinkTarget,double,bool)), m_simVis, SLOT(setPortHighlight(GuiLinkTarget,double,bool)));
    connect(m_animator, SIGNAL(setLinkHighlight(GuiLinkTarget,GuiLinkTarget,double,bool)), m_simVis, SLOT(setLinkHighlight(GuiLinkTarget,GuiLinkTarget,double,bool)));
    connect(m_animator, SIGNAL(commitAnimationFrame()), m_simVis, SLOT(update()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open simulation file");
    if (fileName.isEmpty())
        return;
    try {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            throw tr("Failed to open input file %1").arg(fileName);
        QString text = QString::fromUtf8(f.readAll());
        QScriptValue v = m_jsEngine.evaluate(QString("JSON.parse(%1)").arg(text));
        if (m_jsEngine.hasUncaughtException())
            throw tr("ERROR: %1:%2:\n%3").arg(fileName).arg(m_jsEngine.uncaughtExceptionLineNumber()).arg(v.toString());

        GuiSimulation sim;

        if (!v.isObject())
            throw tr("ERROR: invalid contents in file %1").arg(fileName);
        QScriptValue boxes = v.property("boxes");
        if (!boxes.isArray())
            throw tr("ERROR: Expected property 'boxes' of type Array");
        int boxCount = boxes.property("length").toInt32();
        for(int ibox=0; ibox<boxCount; ++ibox) {
            QScriptValue scriptBox = boxes.property(ibox);
            if (!scriptBox.isObject())
                throw tr("ERROR: boxes[%1] is not an object").arg(ibox);
            QString name = scriptBox.property("name").toString(),
                    type = scriptBox.property("type").toString();
            Box::Ptr box = toBox(scriptBox);
            GuiBox guiBox(type, name);
            guiBox.pos.rx() = scriptBox.property("x").toNumber();
            guiBox.pos.ry() = scriptBox.property("y").toNumber();
            QScriptValue ports = scriptBox.property("ports");
            if (!ports.isArray())
                throw tr("boxes[%i].ports must be an array").arg(ibox);
            QVector<GuiPort> guiPorts;
            foreach(InputPort* port, box->inputPorts())
                guiPorts << GuiPort(port->name(), GuiInputPort);
            foreach(OutputPort* port, box->outputPorts())
                guiPorts << GuiPort(port->name(), GuiOutputPort);
            int portCount = guiPorts.size();
            if (portCount != ports.property("length").toInt32())
                throw tr("Invalid port count in box %1").arg(name);
            for(int iport=0; iport<portCount; ++iport) {
                GuiPort& port = guiPorts[iport];
                port.pos = ports.property(iport).property("pos").toNumber();
                guiBox.ports[port.name] = port;
            }
            sim.boxes[name] = guiBox;
        }

        QScriptValue links = v.property("links");
        if (!links.isArray())
            throw tr("ERROR: Expected property 'links' of type Array");
        int linkCount = links.property("length").toInt32();
        for (int ilink=0; ilink<linkCount; ++ilink)
        {
            QScriptValue scriptLink = links.property(ilink);
            if (!scriptLink.isObject())
                throw tr("ERROR: links[%1] is not an object").arg(ilink);
            sim.links << GuiLink(
                toGuiLinkTarget(scriptLink.property("source")),
                toGuiLinkTarget(scriptLink.property("target")));
        }

        m_sim = sim;
    }
    catch(const QString& msg) {
        QMessageBox::critical(this, QString(), msg);
    }
    m_simVis->update();
}

void MainWindow::commitActivationData(QVector<int>& callerId, ActivationData& d, Animator *animator)
{
    if (d.activatorType != InvalidActivator) {
        animator->beginActivation(d);
        callerId.push_back(d.id);
        d = ActivationData();
    }
}

void MainWindow::finalizeActivationData(QVector<int>& callerId, int id, int result, Animator *animator)
{
    animator->endActivation(id, result);
    Q_ASSERT (callerId.back() == id);
    callerId.pop_back();
}

GuiLinkTarget MainWindow::parseActivatorTarget(const QString& s)
{
    QStringList lst = s.split(":");
    if (lst.size() != 2)
        throw tr("Failed to parse link target %1").arg(s);
    return GuiLinkTarget(lst[0], lst[1]);
}

void MainWindow::openAnimation() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open simulation file");
    if (fileName.isEmpty())
        return;
    try {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            throw tr("Failed to open input file %1").arg(fileName);
        QTextStream t(&f);
        m_animator->clear();
        int lineNumber = 0;
        ActivationData d;
        QVector<int> callerId;
        while (!t.atEnd()) {
            ++lineNumber;
            QString line = t.readLine();
            if (line.isEmpty())
                continue;
            QStringList lst = line.split(" ");
            if (lst.size() < 2)
                throw tr("Syntax error in %1:%2").arg(fileName).arg(lineNumber);
            bool ok;
            int id = lst.first().toInt(&ok);
            if (!(ok && id > 0))
                throw tr("Invalid id in %1:%2").arg(fileName).arg(lineNumber);
            if (lst[1] == "ACTIVATE_FROM_QUEUE") {
                commitActivationData(callerId, d, m_animator);
                callerId.clear();
                ActivatorType type = InvalidActivator;
                GuiLinkTarget target;
                if (lst[2] == "INPUT" && lst[3] == "PORT") {
                    type = InputPortActivator;
                    target = parseActivatorTarget(lst[4]);
                }
                else if (lst[2] == "GENERATOR") {
                    type = GeneratorActivator;
                    target = GuiLinkTarget(lst[3]);
                }
                else if (lst[2] == "PREPROCESSOR") {
                    type = PreprocessorActivator;
                    target = GuiLinkTarget(lst[3]);
                }
                else if (lst[2] == "POSTPROCESSOR") {
                    type = PostprocessorActivator;
                    target = GuiLinkTarget(lst[3]);
                }
                else
                    throw tr("Unknown activator type in %1:%2").arg(fileName).arg(lineNumber);
                d = ActivationData(id, type, target, 0, true);
            }
            else if (lst[1] == "ACTIVATE") {
                commitActivationData(callerId, d, m_animator);
                ActivatorType type = InvalidActivator;
                if (lst[2] == "INPUT" && lst[3] == "PORT")
                    type = InputPortActivator;
                else if (lst[2] == "OUTPUT" && lst[3] == "PORT")
                    type = OutputPortActivator;
                else
                    throw tr("Unknown activator type in %1:%2").arg(fileName).arg(lineNumber);
                GuiLinkTarget target = parseActivatorTarget(lst[4]);
                d = ActivationData(id, type, target, callerId.back());
            }
            else if (lst[1] == "HANDLED")
                d.handled = true;
            else if (lst[1] == "UNHANDLED")
                d.handled = false;
            else if (lst[1] == "LINK")
                continue;   // Seems redundant
            else if (lst[1] == "RESULT") {
                commitActivationData(callerId, d, m_animator);
                bool ok;
                int result = lst[2].toInt(&ok);
                if (!ok)
                    throw tr("Invalid result value in %1:%2").arg(fileName).arg(lineNumber);
                finalizeActivationData(callerId, id, result, m_animator);
            }
        }
    }
    catch(const QString& msg) {
        QMessageBox::critical(this, QString(), msg);
    }
}

Box::Ptr MainWindow::toBox(const QScriptValue& scriptBox)
{
    QString type = scriptBox.property("type").toString();
    Box::Ptr box(BoxFactory::newBox(type));
    // TODO: set properties
    return box;
}

GuiLinkTarget MainWindow::toGuiLinkTarget(const QScriptValue& scriptLinkTarget)
{
    return GuiLinkTarget(
        scriptLinkTarget.property("box").toString(),
        scriptLinkTarget.property("port").toString());
}
