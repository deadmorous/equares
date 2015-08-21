/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <cmath>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include "QVideoEncoder.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->action_Quit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->action_Open, SIGNAL(triggered()), SLOT(openFile()));

    m_simVisWidget = new SimVisualizerWidget();
    SimVisualizer *simVis = m_simVisWidget->simVisualizer();
    Q_ASSERT(simVis);
    simVis->setSimulation(&m_sim);
    setCentralWidget(m_simVisWidget);

    m_animator = new Animator(this);
    connect(ui->actionOpen_animation, SIGNAL(triggered()), SLOT(openAnimation()));
    connect(ui->action_Start, SIGNAL(triggered()), m_animator, SLOT(play()));
    connect(ui->actionStop, SIGNAL(triggered()), m_animator, SLOT(stop()));
    connect(ui->actionRecord_video, SIGNAL(triggered()), SLOT(recordVideo()));
    connect(ui->action_Record_PNG_sequence, SIGNAL(triggered()), SLOT(recordPngImages()));
    connect(ui->actionRe_wind_to_the_beginning, SIGNAL(triggered()), SLOT(rewindToTheBeginning()));

    connect(m_animator, SIGNAL(setBoxHighlight(QString,double,bool)), simVis, SLOT(setBoxHighlight(QString,double,bool)));
    connect(m_animator, SIGNAL(setPortHighlight(GuiLinkTarget,double,bool)), simVis, SLOT(setPortHighlight(GuiLinkTarget,double,bool)));
    connect(m_animator, SIGNAL(setLinkHighlight(GuiLinkTarget,GuiLinkTarget,double,bool)), simVis, SLOT(setLinkHighlight(GuiLinkTarget,GuiLinkTarget,double,bool)));
    connect(m_animator, SIGNAL(commitAnimationFrame()), m_simVisWidget, SLOT(update()));
    connect(m_animator, SIGNAL(startAnimation()), simVis, SLOT(startAnimation()));
    connect(m_animator, SIGNAL(endAnimation()), simVis, SLOT(endAnimation()));
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
        QScriptValue v = m_jsEngine.evaluate(QString("JSON.parse(%1.definition)").arg(text));
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
        m_simVisWidget->simVisualizer()->setSimulation(&m_sim);
    }
    catch(const QString& msg) {
        QMessageBox::critical(this, QString(), msg);
    }
    m_simVisWidget->update();
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

GuiLinkTarget MainWindow::parseActivatorTarget(const QStringList& tokens, int idx)
{
    QString s = QStringList(tokens.mid(idx)).join(" ");
    QStringList lst = s.split(":");
    if (lst.size() != 2)
        throw tr("Failed to parse link target %1").arg(s);
    return GuiLinkTarget(lst[0], lst[1]);
}

void MainWindow::openAnimation()
{
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
                    target = parseActivatorTarget(lst, 4);
                }
                else if (lst[2] == "OUTPUT" && lst[3] == "PORT") {
                    type = OutputPortActivator;
                    target = parseActivatorTarget(lst, 4);
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
                GuiLinkTarget target = parseActivatorTarget(lst, 4);
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

void MainWindow::rewindToTheBeginning()
{
    m_animator->stop();
    m_animator->setTimeStep(0);
}

static void image2Pixmap(const QImage &img,QPixmap &pixmap)
{
   // Convert the QImage to a QPixmap for display
   pixmap = QPixmap(img.size());
   QPainter painter;
   painter.begin(&pixmap);
   painter.drawImage(0,0,img);
   painter.end();
}

void MainWindow::recordVideo()
{
    QString filename = "test.mpeg";
    bool vfr = false;

//    int width=320;
//    int height=240;
    int width=640;
    int height=480;
    int bitrate=2000000;
    int gop = 5;
    int fps = 25;

    // The image on which we draw the frames
    QImage frame(width,height,QImage::Format_RGB32);     // Only RGB32 is supported

    // A painter to help us draw
    QPainter painter(&frame);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create the encoder
    QVideoEncoder encoder;
    if(!vfr)
       encoder.createFile(filename,width,height,bitrate,gop,fps);        // Fixed frame rate
    else
       encoder.createFile(filename,width,height,bitrate*1000/fps,gop,1000);  // For variable frame rates: set the time base to e.g. 1ms (1000fps),
                                                                            // and correct the bitrate according to the expected average frame rate (fps)

    // QEventLoop evt;      // we use an event loop to allow for paint events to show on-screen the generated video
    SimVisualizer *simVis = m_simVisWidget->simVisualizer();
    Q_ASSERT(simVis);
    m_animator->stop();
    m_animator->setTimeStep(0);
    simVis->endAnimation();
    simVis->startAnimation();

    // Generate a few hundred frames
    int size=0;
    int maxframe=1000;
    unsigned pts=0;
    for(int i=0;i<maxframe && m_animator->timeStep() < m_animator->timeStepCount();i++)
    {
       // Clear the frame
       painter.fillRect(frame.rect(),Qt::white);

       // Draw a moving square
       // painter.fillRect(width*i/maxframe,height*i/maxframe,30,30,Qt::blue);
       simVis->paint(&painter, frame.rect());
       m_animator->nextAnimationFrame();

       // Frame number
       // painter.drawText(frame.rect(),Qt::AlignCenter,QString("Frame %1\nLast frame was %2 bytes").arg(i).arg(size));

       // Display the frame, and processes events to allow for screen redraw
       QPixmap p;
       image2Pixmap(frame,p);
       // ui->labelVideoFrame->setPixmap(p);
       // evt.processEvents();

       if(!vfr)
          size=encoder.encodeImage(frame);                      // Fixed frame rate
       else
       {
          // Variable frame rate: the pts of the first frame is 0,
          // subsequent frames slow down
          pts += sqrt(i);
          if(i==0)
             size=encoder.encodeImagePts(frame,0);
          else
             size=encoder.encodeImagePts(frame,pts);
       }

       printf("Encoded: %d\n",size);
    }

    encoder.close();
}

static QString paddedNumber(int n, int size)
{
    QString s = QString::number(n);
    size -= s.size();
    if (size > 0)
        s = QString(size, QChar::fromLatin1('0')) + s;
    return s;
}

void MainWindow::recordPngImages()
{
    QDir("png").removeRecursively();
    QDir dir;
    if (!dir.mkdir("png"))
        return;
    dir.cd("png");

    int width=320;
    int height=240;
    //    int width=640;
    //    int height=480;

    // The image on which we draw the frames
    QImage frame(width,height,QImage::Format_RGB32);     // Only RGB32 is supported

    // A painter to help us draw
    QPainter painter(&frame);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);

    // QEventLoop evt;      // we use an event loop to allow for paint events to show on-screen the generated video
    SimVisualizer *simVis = m_simVisWidget->simVisualizer();
    Q_ASSERT(simVis);
    m_animator->stop();
    m_animator->setTimeStep(0);
    m_animator->setStepSubdivision(3);
    simVis->endAnimation();
    simVis->startAnimation();

    // Generate a few hundred frames
    int maxframe=999;
    for(int i=0;i<maxframe && m_animator->timeStep() < m_animator->timeStepCount();i++)
    {
       // Clear the frame
       painter.fillRect(frame.rect(),Qt::white);

       simVis->paint(&painter, frame.rect());
       m_animator->nextAnimationFrame();
       QString fileName = QString("frame_%1.png").arg(paddedNumber(i+1, 4));
       QString filePath = dir.absoluteFilePath(fileName);
       frame.save(filePath);

       // Frame number
       // painter.drawText(frame.rect(),Qt::AlignCenter,QString("Frame %1\nLast frame was %2 bytes").arg(i).arg(size));

       qDebug() << "Saved frame " << i << " to file " << filePath;
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
