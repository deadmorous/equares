/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "equares_core.h"
#include <QSet>
#include <QScriptEngine>

#ifdef EQUARES_DUMP_SIMULATION_LOG
QTextStream& equaresSimLog()
{
    static QFile f("equares_simulation_log.txt");
    static QTextStream s(&f);
    static bool open = false;
    if (!open) {
        open = true;
        f.open(QIODevice::WriteOnly);
    }
    return s;
}

int equaresNextLogOperationId()
{
    static int id = 0;
    return ++id;
}

#endif // EQUARES_DUMP_SIMULATION_LOG

QString Box::decoratedName() const {
    return simulation()->decoratedBoxName(this);
}

void Box::throwBoxException(const QString& what) const
{
    throw EquaresBoxException(this, what);
}

void Simulation::initPortFormat()
{
    // Clear all non-fixed formats; check that fixed formats are valid
    foreach (const Link& link, m_links) {
        if (!link.inputPort()->format().isFixed())
            link.inputPort()->format().setValid(false);
        else {
            Q_ASSERT(link.inputPort()->format().isValid());
        }
    }

    BoxList allBoxes = boxes();
    forever {
        bool progress = false;

        bool hasUnresolvedBoxes = false;

        // Propagate all valid formats according to links
        foreach (const Link& link, m_links) {
            PortFormat
                &f1 = link.inputPort()->format(),
                &f2 = link.outputPort()->format();

            // deBUG, TODO: Remove
            QString linkstr = QString("%1:%2 -> %3:%4").arg(
                    link.outputPort()->owner()->name(),
                    link.outputPort()->name(),
                    link.inputPort()->owner()->name(),
                    link.inputPort()->name());
            Q_UNUSED(linkstr);

            if (f1.isValid()) {
                if (f2.isValid()) {
                    if (f1 != f2) {
                        throw EquaresException(QString
                            ("Incompatible port formats at link %1:%2 -> %3:%4").arg(
                                link.outputPort()->owner()->name(),
                                link.outputPort()->name(),
                                link.inputPort()->owner()->name(),
                                link.inputPort()->name()));
                    }
                }
                else {
                    f2 = f1;
                    progress = true;
                }
            }
            else if (f2.isValid()) {
                f1 = f2;
                progress = true;
            }
            else
                hasUnresolvedBoxes = true;
        }
        if (!hasUnresolvedBoxes)
            // Done
            break;

        // Propagate format through boxes
        foreach (Box *box, allBoxes)
            progress = box->propagatePortFormat() || progress;

        if (!progress)
            // Finished
            break;
    }

    // Check final port formats
    foreach (Box *box, allBoxes)
        box->checkPortFormat();
}

QString Simulation::decoratedBoxName(const Box *box) const {
    BoxDecoratedNameMap::const_iterator it = m_decoratedBoxNames.find(box);
    if (it == m_decoratedBoxNames.end()) {
        QString baseName = box->name();
        baseName.replace(QRegExp("\\s+"), "_");
        QString  name = baseName;
        int n = 1;
        while(m_idecoratedBoxNames.contains(name)) {
            name = baseName + "_" + QString::number(n);
            ++n;
        }
        m_idecoratedBoxNames[name] = box;
        m_decoratedBoxNames[box] = name;
        return name;
    }
    else
        return it.value();
}

bool Simulation::hasUnknownInputFormats(const Box *box)
{
    foreach (InputPort* port, box->inputPorts())
        if (!port->format().isValid())
            return true;
    return false;
}

void Simulation::setLinks(const LinkList& links)
{
    try {
        // Clear boxes' links; set simulation
        foreach (Box *box, linkBoxes(links)) {
            if (!m_boxes.contains(box))
                m_boxes << box;
            foreach (InputPort *port, box->inputPorts())
                port->setLink(0);
            foreach (OutputPort *port, box->outputPorts())
                port->links().clear();
            box->setSimulation(this);
        }

        // Add specified links
        foreach (const Link& link, links)
            addLink(link);

        // Resolve port formats
        initPortFormat();
    } catch(const EquaresException& e) {
        context()->throwError(e.what());
    }
}

void Simulation::setBoxes(const QScriptValue& boxes)
{
    BoxList newBoxes;
    if (!boxes.isArray()) {
        context()->throwError("Simulation::setBoxes: invalid argument (an array was expected)");
        return;
    }
    int n = boxes.property("length").toInt32();
    for (int i=0; i<n; ++i) {
        QScriptValue jsbox = boxes.property(i);
        if (jsbox.isQObject()) {
            Box *box = qobject_cast<Box*>(jsbox.toQObject());
            if (box) {
                box->setSimulation(this);
                newBoxes << box;
                continue;
            }
        }
        context()->throwError("Simulation::setBoxes: invalid argument (an array of boxes was expected)");
        return;
    }
    m_decoratedBoxNames.clear();
    m_idecoratedBoxNames.clear();
    m_boxes = newBoxes;
}

BoxList Simulation::linkBoxes(const LinkList& links)
{
    BoxList result;
    foreach (const Link& link, links) {
        if (!result.contains(link.inputPort()->owner()))
            result << link.inputPort()->owner();
        if (!result.contains(link.outputPort()->owner()))
            result << link.outputPort()->owner();
    }
    return result;
}

void Simulation::run() const
{
    try {
        Runner *runner = new Runner;
        runner->start(this);
    } catch(const EquaresException& e) {
        context()->throwError(e.what());
    }
}



void Runner::start(const Simulation *sim)
{
    // Clear any previous runtime configuration
    m_rtboxes.clear();
    m_rtlinks.clear();

    // Create runtime configuration
    QMap<const Box*, RuntimeBox*> box2rt;
    foreach (Box *box, sim->boxes()) {
        RuntimeBox *rtbox = box->newRuntimeBox();
        rtbox->setRunner(this);
        box2rt[box] = rtbox;
        m_rtboxes << RuntimeBox::Ptr(rtbox);
    }
    m_rtboxvec.resize(m_rtboxes.size());
    for (int i=0; i<m_rtboxes.size(); ++i)
        m_rtboxvec[i] = m_rtboxes[i].data();
    foreach (const Link& link, sim->links()) {
        const Box
            *boxin = link.inputPort()->owner(),
            *boxout = link.outputPort()->owner();
        const RuntimeBox
            *rtboxin = box2rt.value(boxin),
            *rtboxout = box2rt.value(boxout);
        int iin = boxin->inputPorts().indexOf(link.inputPort()),
            iout = boxout->outputPorts().indexOf(link.outputPort());
        RuntimeInputPort *in = rtboxin->inputPorts()[iin];
        RuntimeOutputPort *out = rtboxout->outputPorts()[iout];
        m_rtlinks << RuntimeLink()
            .setInputPort(in)
            .setOutputPort(out);
        RuntimeLink *rtlink = &m_rtlinks.last();
        in->setLink(rtlink);
        out->links() << rtlink;
    }

    // Start simulation
    ThreadManager::instance()->start(this);
}

void Runner::run()
{
    // Declare short name for EQUARES_COUT
    QTextStream& out = EQUARES_COUT;

    // Register and announce interactive input
    out << "begin input announcement" << endl;
    foreach (RuntimeBox::Ptr box, m_rtboxes) {
        foreach (InputInfo::Ptr info, box->inputInfo())
            out << info->toString() << endl;
        box->registerInput();
    }
    out << "end input announcement" << endl;

    // Announce output files
    OutputFileInfoList ofi;
    {
        // Gather output file information from all boxes
        foreach (const RuntimeBox::Ptr& box, m_rtboxes)
            ofi << box->outputFileInfo();

        // Check that there are no duplicate output file names
        QSet<QString> fileNames;
        foreach(const OutputFileInfo& info, ofi)
            if (fileNames.contains(info.name()))
                throw EquaresException(QObject::tr("Duplicate output file name '%1'").arg(info.name()));
            else
                fileNames << info.name();

        // Generate output file stubs
        foreach(const OutputFileInfo& info, ofi)
            info.createStubFile();

        out << "begin file announcement" << endl;
        foreach(const OutputFileInfo& info, ofi)
            out << info.toString() << endl;
        out << "end file announcement" << endl;
    }

    // Initialize box queue
    initQueue();

    // Process enqueued boxes
    m_terminationRequested = 0;
    int runFailures = 0;
    bool postProcessorsQueued = false;
    while (!m_queue.isEmpty()) {
        if (terminationRequested())
            break;
        try {
            bool ok = m_queue.first().activate();
            if (!ok) {
                const int BoxFailureLimit = 100;
                if (++runFailures > m_rtboxes.size() + BoxFailureLimit)
                    throw EquaresException("Failed to start simulation. Please check the scheme.");
                foreach (const RuntimeBox::Ptr& box, m_rtboxes)
                    box->reset();
                m_queue << m_queue.first();
            }
            m_queue.removeFirst();
            if (ok && !postProcessorsQueued && m_queue.isEmpty()) {
                // Enqueue postrocessors
                postProcessorsQueued = true;
                foreach (RuntimeBox::Ptr box, m_rtboxes) {
                    RuntimeBox::PortNotifier postprocessor = box->postprocessor();
                    if (postprocessor)
#ifdef EQUARES_DUMP_SIMULATION_LOG
                        postPortActivation(box.data(), postprocessor, 0, "POSTPROCESSOR " + box->owner()->name());
#else // EQUARES_DUMP_SIMULATION_LOG
                        postPortActivation(box.data(), postprocessor, 0);
#endif // EQUARES_DUMP_SIMULATION_LOG
                }
            }
        }
        catch(const BoxBreakException& bbx) {
            m_queue.clear();
            postProcessorsQueued = false;
            foreach (const RuntimeBox::Ptr& box, m_rtboxes)
                box->restart();
            RuntimeBox *box = bbx.rtbox();
            if(box) {
                if (box->generator()) {
                    Q_ASSERT(box->inputPorts().empty());
#ifdef EQUARES_DUMP_SIMULATION_LOG
                        postPortActivation(box, box->generator(), 0, "GENERATOR " + box->owner()->name());
#else // EQUARES_DUMP_SIMULATION_LOG
                        postPortActivation(box, box->generator(), 0);
#endif // EQUARES_DUMP_SIMULATION_LOG
                }
                else {
                    foreach (const RuntimeOutputPort *port, box->outputPorts())
                        foreach (const RuntimeLink *link, port->links()) {
                            RuntimeInputPort *inport = link->inputPort();
#ifdef EQUARES_DUMP_SIMULATION_LOG
                            postPortActivation(inport->owner(), inport->portNotifier(), inport->portId(), inport->toString());
#else // EQUARES_DUMP_SIMULATION_LOG
                            postPortActivation(inport->owner(), inport->portNotifier(), inport->portId());
#endif // EQUARES_DUMP_SIMULATION_LOG
                        }
                }
            }
            else
                initQueue();
        }
    }

    // Report final state of output files
    foreach(const OutputFileInfo& info, ofi)
        out << "file: " << info.name() << endl;
}

void Runner::initQueue()
{
    // clear queue
    m_queue.clear();

    // Enqueue preprocessors
    foreach (RuntimeBox::Ptr box, m_rtboxes) {
        RuntimeBox::PortNotifier preprocessor = box->preprocessor();
        if (preprocessor)
#ifdef EQUARES_DUMP_SIMULATION_LOG
            postPortActivation(box.data(), preprocessor, 0, "PREPROCESSOR " + box->owner()->name());
#else // EQUARES_DUMP_SIMULATION_LOG
            postPortActivation(box.data(), preprocessor, 0);
#endif // EQUARES_DUMP_SIMULATION_LOG
    }

    // Enqueue regular sources
    foreach (const RuntimeBox::Ptr& box, m_rtboxes)
        if (!box->generator() && box->inputPorts().empty())
            foreach (const RuntimeOutputPort *port, box->outputPorts())
                foreach (const RuntimeLink *link, port->links()) {
                    RuntimeInputPort *inport = link->inputPort();
#ifdef EQUARES_DUMP_SIMULATION_LOG
                    postPortActivation(inport->owner(), inport->portNotifier(), inport->portId(), inport->toString());
#else // EQUARES_DUMP_SIMULATION_LOG
                    postPortActivation(inport->owner(), inport->portNotifier(), inport->portId());
#endif // EQUARES_DUMP_SIMULATION_LOG
                }

    // Enqueue generators
    foreach (const RuntimeBox::Ptr& box, m_rtboxes)
        if (box->generator()) {
            Q_ASSERT(box->inputPorts().empty());
#ifdef EQUARES_DUMP_SIMULATION_LOG
            postPortActivation(box.data(), box->generator(), 0, "GENERATOR " + box->owner()->name());
#else // EQUARES_DUMP_SIMULATION_LOG
            postPortActivation(box.data(), box->generator(), 0);
#endif // EQUARES_DUMP_SIMULATION_LOG
        }
}

void Runner::requestTermination() {
    m_terminationRequested = 1;
}

bool Runner::terminationRequested() const {
#if QT_VERSION >= 0x050000
    int terminationRequested = m_terminationRequested.load();
#else
    int terminationRequested = m_terminationRequested;
#endif
    return terminationRequested != 0;
}

#ifdef EQUARES_DUMP_SIMULATION_LOG
void Runner::postPortActivation(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg, const QString& itemId) {
    Q_ASSERT(box);
    if (notifier)
        m_queue << QueueItem(box, notifier, notifierArg, itemId);
}
#else // EQUARES_DUMP_SIMULATION_LOG
void Runner::postPortActivation(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg) {
    Q_ASSERT(box);
    if (notifier)
        m_queue << QueueItem(box, notifier, notifierArg);
}
#endif // EQUARES_DUMP_SIMULATION_LOG

const QVector<RuntimeBox *> &Runner::rtboxes() const {
    return m_rtboxvec;
}



BoxFactory::BoxMap *BoxFactory::m_registry = 0;

Box *BoxFactory::newBox(const QString& name) {
    return boxCtor(name)();
}

Box::Ctor BoxFactory::boxCtor(const QString& name) {
    Box::Ctor ctor = registry().value(name);
    if (!ctor)
        throw EquaresException(QString("Unknown box type '%1'").arg(name));
    return ctor;
}

void BoxFactory::registerBoxType(const QString& name, Box::Ctor ctor) {
    BoxMap& r = registry();
    if (r.contains(name))
        throw EquaresException(QString("Box type '%1' is already registered").arg(name));
    r[name] = ctor;
}

QStringList BoxFactory::boxTypes() {
    return registry().keys();
}

BoxFactory::BoxMap& BoxFactory::registry() {
    if (!m_registry)
        m_registry = new BoxMap;
    return *m_registry;
}
