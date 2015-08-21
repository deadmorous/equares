/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef EQUARES_CORE_H
#define EQUARES_CORE_H

#include <QObject>
#include <QScriptable>
#include <QScriptContext>
#include <QVector>
#include <QList>
#include <QStringList>
#include <QSharedPointer>
#include <QMultiMap>
#include <QAtomicInt>
#include <QSettings>

#include "equares_core_global.h"
#include "EquaresException.h"
#include "equares_exec.h"

// #define EQUARES_DUMP_SIMULATION_LOG

#ifdef EQUARES_DUMP_SIMULATION_LOG
QTextStream& equaresSimLog();
int equaresNextLogOperationId();
#endif // EQUARES_DUMP_SIMULATION_LOG

class Box;
class Port;
class InputPort;
class OutputPort;
typedef QList<InputPort*> InputPorts;
typedef QList<OutputPort*> OutputPorts;
typedef LinkTemplate<InputPort, OutputPort> Link;
typedef QList<const Link*> ConstLinkList;

class RuntimeBox;
class RuntimePort;
class RuntimeInputPort;
class RuntimeOutputPort;
typedef QList<RuntimeInputPort*> RuntimeInputPorts;
typedef QList<RuntimeOutputPort*> RuntimeOutputPorts;
typedef LinkTemplate<RuntimeInputPort, RuntimeOutputPort> RuntimeLink;

class Simulation;
class Runner;

class PortFormat
{
public:
    PortFormat() : m_flags(0) {}
    explicit PortFormat(const QVector<int>& size) : m_size(size), m_flags(PortFormatValid) {}
    explicit PortFormat(int length) : m_size(1, length), m_flags(PortFormatValid) {}
    PortFormat(int length1, int length2) : m_size(2), m_flags(PortFormatValid) {
        m_size[0] = length1;
        m_size[1] = length2;
    }
    int dimension() const { return m_size.size(); }
    const QVector<int>& size() const { return m_size; }
    int size(int index) const {
        Q_ASSERT(index >= 0   &&   index < m_size.size());
        return m_size[index];
    }
    bool isValid() const {
        return hasFlag(m_flags, PortFormatValid);
    }
    PortFormat& setValid(bool valid = true) {
        setFlag(m_flags, PortFormatValid, valid);
        return *this;
    }
    bool isFixed() const {
        return hasFlag(m_flags, PortFormatFixed);
    }
    PortFormat& setFixed(bool fixed = true) {
        setFlag(m_flags, PortFormatFixed, fixed);
        return *this;
    }

    PortFormat& setDimension(int dimension) {
        m_size = QVector<int>(dimension, 0);
        setValid(false);
        return *this;
    }
    PortFormat& setSize(const QVector<int>& size) {
        m_size = size;
        setValid();
        return *this;
    }
    PortFormat& setSize(int length) {
        m_size = QVector<int>(1, length);
        setValid();
        return *this;
    }
    PortFormat& setSize(int index, int length) {
        Q_ASSERT(index >= 0   &&   index < m_size.size());
        m_size[index] = length;
        return *this;
    }
    bool isEempty() const {
        return m_size.isEmpty();
    }
    int dataSize() const {
        int result = 1;
        foreach(int s, m_size)
            result *= s;
        return result;
    }
    int flattenIndex(const int *multiIndex) const {
        int result = 0,   m = 1;
        for(int i=m_size.size()-1; i>=0; m*=m_size[i], --i) {
            Q_ASSERT(multiIndex[i] >= 0   &&   multiIndex[i] < m_size[i]);
            result += m*multiIndex[i];
        }
        Q_ASSERT(result >= 0   &&   result < dataSize());
        return result;
    }
    bool operator==(const PortFormat& that) const {
        if (isValid() != that.isValid())
            return false;
        return !isValid()   ||   m_size == that.m_size;
    }
    bool operator!=(const PortFormat& that) const {
        return !(*this == that);
    }
private:
    enum PortFormatFlags {
        PortFormatValid = 0x01,
        PortFormatFixed = 0x02
    };

    QVector<int> m_size;
    int m_flags;
};

PropRefClass(WithPortFormat, PortFormat, format)

PropClass(WithSimulation, Simulation*, Simulation*, simulation, setSimulation)

PropClass(WithHelpString, QString, const QString&, helpString, setHelpString)

PropClass(WithPortId, int, int, portId, setPortId)



class EQUARES_CORESHARED_EXPORT EntryHints
{
public:
    bool hasEntryHints() const {
        return !m_entryHints.isEmpty();
    }
    QStringList entryHints() const {
        return m_entryHints;
    }
    EntryHints& setEntryHints(const QStringList& entryHints) {
        m_entryHints = entryHints;
        return *this;
    }
    EntryHints& loadSettings(QSettings& settings);
private:
    QStringList m_entryHints;
};

PropRefClass(WithEntryHints, EntryHints, hints)

class EQUARES_CORESHARED_EXPORT PortHints : public EntryHints
{
public:
    PortHints() : m_position(-1) {}
    bool hasPosition() const {
        return m_position >= 0;
    }
    double position() const {
        return m_position;
    }
    PortHints& setPosition(double position) {
        m_position = position;
        return *this;
    }
    PortHints& loadSettings(QSettings& settings);
private:
    double m_position;
};

PropRefClass(WithPortHints, PortHints, hints)

struct EQUARES_CORESHARED_EXPORT BoxProperty
{
    BoxProperty() {}
    explicit BoxProperty(const QString& name, const QString& helpString) :
        name(name), helpString(helpString), critical(false) {}
    QString name;
    QString helpString;
    QString userType;
    QString toUserType;
    QString toBoxType;
    QStringList deps;
    QString resolveUserType;
    bool critical;
    QString snippet;
    BoxProperty& loadSettings(QSettings& settings);
};
typedef QList<BoxProperty> BoxPropertyList;

PropClass(WithBoxProps, BoxPropertyList, const BoxPropertyList&, boxProperties, setBoxProperties)

class EQUARES_CORESHARED_EXPORT Box :
    public QObject,
    protected QScriptable,
    public Named,
    public Typed,
    public WithSimulation,
    public WithHelpString,
    public WithBoxProps
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString type READ type)

public:
    typedef QSharedPointer<Box> Ptr;

    typedef Box* (*Ctor)();

    explicit Box(QObject *parent = 0) : QObject(parent) {}
    virtual ~Box() {}

    virtual InputPorts inputPorts() const = 0;
    virtual OutputPorts outputPorts() const = 0;
    virtual void checkPortFormat() const = 0;
    virtual bool propagatePortFormat() = 0;

    virtual RuntimeBox *newRuntimeBox() const = 0;

    Box& loadSettings(QSettings& settings);
    QString decoratedName() const;

protected:
    void throwBoxException(const QString& what) const;

private:
};

class EquaresBoxException : public EquaresException
{
public:
    ~EquaresBoxException() throw() {}
    explicit EquaresBoxException(const Box *box, const QString& what) throw() : EquaresException(what), m_box(box) {}
    const char *what() const throw() {
        QString boxName;
        if (m_box)
            boxName = m_box->name();
        if (boxName.isEmpty())
            m_buf = EquaresException::what();
        else
            m_buf = ("[box='" + boxName + "'] ").toUtf8() + EquaresException::what();
        return m_buf.constData();
    }
private:
    const Box *m_box;
    mutable QByteArray m_buf;
 };

class EQUARES_CORESHARED_EXPORT Port :
    public OwnedBy<Box>,
    public WithPortFormat,
    public Named,
    public WithHelpString,
    public WithPortHints
{
public:
    Port() {}
    virtual ~Port() {}
    Port(const QString& name, Box *owner, const PortFormat& format = PortFormat()) :
        OwnedBy<Box>(owner), WithPortFormat(format), Named(name)
    {}
    Port& loadSettings(QSettings& settings);
};

class InputPort :
    public Port,
    public LinkedToOne<const Link>
{
public:
    InputPort() {}
    InputPort(const QString& name, Box *owner, const PortFormat& format = PortFormat()) :
        Port(name, owner, format) {}
};

class OutputPort :
    public Port,
    public LinkedToMany<const Link>
{
public:
    OutputPort() {}
    OutputPort(const QString& name, Box *owner, const PortFormat& format = PortFormat()) :
        Port(name, owner, format) {}
};




class PortState
{
public:
    PortState() : m_flags(0) {}
    bool isOpen() const { return (m_flags & PortClosed) == 0; }
    bool isClosed() const { return (m_flags & PortClosed) != 0; }
    bool hasData() const { return (m_flags & PortDataValid) != 0; }
    PortState& open() {
        clearFlag(m_flags, PortClosed);
        return *this;
    }
    PortState& close() {
        setFlag(m_flags, PortClosed);
        return *this;
    }
    PortState& setValid() {
        setFlag(m_flags, PortDataValid);
        return *this;
    }
    PortState& invalidate() {
        clearFlag(m_flags, PortDataValid);
        return *this;
    }
private:
    enum PortStateFlags {
        PortDataValid = 0x01,
        PortClosed = 0x02
    };
    int m_flags;
};

class PortData
{
public:
    PortData() : m_size(0), m_data(0) {}
    PortData(int size, double *data) : m_size(size), m_data(data) {}
    int size() const { return m_size; }
    double *data() const {
        return m_data;
    }
    operator double*() const {
        return m_data;
    }
    void copyFrom(const double *data) const {
        for (int i=0; i<m_size; ++i)
            m_data[i] = data[i];
    }
    void copyTo(double *data) const {
        for (int i=0; i<m_size; ++i)
            data[i] = m_data[i];
    }

private:
    int m_size;
    double *m_data;
};

PropClass(WithPort, const Port*, const Port*, port, setPort)
PropRefClass(WithPortState, PortState, state)
PropClass(WithPortData, PortData, const PortData&, data, setData)

PropClass(WithRuntimeInputPorts, RuntimeInputPorts, const RuntimeInputPorts&, inputPorts, setInputPorts)
PropClass(WithRuntimeOutputPorts, RuntimeOutputPorts, const RuntimeOutputPorts&, outputPorts, setOutputPorts)
PropClass(WithRunner, Runner*, Runner*, runner, setRunner)
PropClass(WithBusyFlag, bool, bool, isBusy, setBusy)

typedef InitializedInt<-1> InputId;

class RuntimeBox :
    public OwnedBy<const Box>,
    public WithRuntimeInputPorts,
    public WithRuntimeOutputPorts,
    public WithRunner
{
public:
    typedef QSharedPointer<RuntimeBox> Ptr;
    virtual ~RuntimeBox () {}
    typedef bool (RuntimeBox::*PortNotifier)(int);
    virtual OutputFileInfoList outputFileInfo() const {
        return OutputFileInfoList();
    }
    virtual InputInfoList inputInfo() const {
        return InputInfoList();
    }
    virtual void registerInput() {}
    virtual PortNotifier generator() const {
        return 0;
    }
    virtual PortNotifier preprocessor() const {
        return 0;
    }
    virtual PortNotifier postprocessor() const {
        return 0;
    }
    virtual void reset() {}
    virtual void restart() {}
    virtual void acquireInteractiveInput() {}

protected:
    template<class ThisClass>
    static PortNotifier toPortNotifier(bool (ThisClass::*notifier)(int)) {
        return static_cast<PortNotifier>(notifier);
    }
    void throwBoxException(const QString& what) const {
        throw EquaresBoxException(owner(), what);
    }

};

class RuntimePort :
    public OwnedBy<RuntimeBox>,
    public WithPort
{
public:
    void init(RuntimeBox *owner, Port *port) {
        setOwner(owner);
        setPort(port);
    }
};

PropClass(WithPortNotifier, RuntimeBox::PortNotifier, RuntimeBox::PortNotifier, portNotifier, setPortNotifier)

class RuntimeInputPort :
    public RuntimePort,
    public LinkedToOne<const RuntimeLink>,
    public WithPortNotifier,
    public WithPortId
{
public:
    void init(RuntimeBox *owner, Port *port, RuntimeBox::PortNotifier portNotifier = 0, int portId = 0) {
        RuntimePort::init(owner, port);
        setPortNotifier(portNotifier);
        setPortId(portId);
    }

    bool hasPortNotifier() const {
        return portNotifier() != 0;
    }
    inline bool activate() const;

    inline RuntimeOutputPort *outputPort() {
        return link()->outputPort();
    }
    inline const RuntimeOutputPort *outputPort() const {
        return link()->outputPort();
    }

    inline const PortData& data() const;
    inline RuntimeInputPort& setData(const PortData& data);

    inline PortState& state();
    inline const PortState& state() const;

#ifdef EQUARES_DUMP_SIMULATION_LOG
    QString toString() const {
        return "INPUT PORT " + port()->owner()->name() + ":" + port()->name();
    }
#endif // EQUARES_DUMP_SIMULATION_LOG
};

class RuntimeOutputPort :
    public RuntimePort,
    public LinkedToMany<const RuntimeLink>,
    public WithPortState,
    public WithPortData
{
public:
    void init(RuntimeBox *owner, Port *port, const PortData& data = PortData()) {
        RuntimePort::init(owner, port);
        setData(data);
    }

    bool hasActiveLinks() const {
        foreach (const RuntimeLink *link, links())
            if(link->inputPort()->hasPortNotifier())
                return true;
        return false;
    }
    bool activateLinks() const {
#ifdef EQUARES_DUMP_SIMULATION_LOG
        int id = equaresNextLogOperationId();
        equaresSimLog() << id << " ACTIVATE " << toString() << endl;
#endif // EQUARES_DUMP_SIMULATION_LOG
        bool result = true;
        foreach (const RuntimeLink *link, links()) {
#ifdef EQUARES_DUMP_SIMULATION_LOG
        equaresSimLog() << id << " LINK TO " << link->inputPort()->toString() << endl;
#endif // EQUARES_DUMP_SIMULATION_LOG
            result = link->inputPort()->activate() && result;
        }
#ifdef EQUARES_DUMP_SIMULATION_LOG
        equaresSimLog() << id << " RESULT " << result << endl;
#endif // EQUARES_DUMP_SIMULATION_LOG
        return result;
    }
#ifdef EQUARES_DUMP_SIMULATION_LOG
    QString toString() const {
        return "OUTPUT PORT " + port()->owner()->name() + ":" + port()->name();
    }
#endif // EQUARES_DUMP_SIMULATION_LOG
};

inline const PortData& RuntimeInputPort::data() const {
    return link()->outputPort()->data();
}

inline RuntimeInputPort& RuntimeInputPort::setData(const PortData& data) {
    link()->outputPort()->setData(data);
    return *this;
}

inline PortState& RuntimeInputPort::state() {
    return link()->outputPort()->state();
}

inline const PortState& RuntimeInputPort::state() const {
    return link()->outputPort()->state();
}

typedef QList< Box* > BoxList;
typedef QList< Link > LinkList;

class EQUARES_CORESHARED_EXPORT Simulation :
    public QObject,
    protected QScriptable
{
    Q_OBJECT
public:
    explicit Simulation(QObject *parent = 0) : QObject(parent) {}
    BoxList boxes() const {
        return m_boxes;
    }
    const LinkList& links() const {
        return m_links;
    }

    const Link *findLink(OutputPort *outputPort, InputPort *inputPort) const
    {
        Q_ASSERT(outputPort);
        Q_ASSERT(inputPort);
        const Link *link = inputPort->link();
        if (link   &&   link->outputPort() == outputPort)
            return link;
        return 0;
    }
    bool isConnected(OutputPort *outputPort, InputPort *inputPort) const
    {
        return findLink(outputPort, inputPort) != 0;
    }
    Simulation& addLink(const Link& link)
    {
        Q_ASSERT(!link.inputPort()->isConnected());
        m_links << link;
        const Link *l = &m_links.last();
        link.inputPort()->setLink(l);
        OutputPort *o = link.outputPort();
        o->links() << l;
        return *this;
    }
    Simulation& removeLink(const Link& link)
    {
        Q_ASSERT(isConnected(link.outputPort(), link.inputPort()));
        int i = m_links.indexOf(link);
        Q_ASSERT(i != -1);
        const Link *l = &m_links[i];
        ConstLinkList& portLinks = l->outputPort()->links();
        int ip = portLinks.indexOf(l);
        Q_ASSERT(ip!= -1);
        portLinks.removeAt(ip);
        Q_ASSERT(l->inputPort()->isConnected());
        l->inputPort()->setLink(0);
        m_links.removeAt(i);
        return *this;
    }

    void initPortFormat();

    QString decoratedBoxName(const Box *box) const;

    using QScriptable::engine;

public slots:
    void setLinks(const LinkList& links);
    void setBoxes(const QScriptValue& boxes);
    void run() const;

private:
    LinkList m_links;
    BoxList m_boxes;
    typedef QMap<const Box*, QString> BoxDecoratedNameMap;
    typedef QMap<QString, const Box*> BoxDecoratedNameIMap;
    mutable BoxDecoratedNameMap m_decoratedBoxNames;
    mutable BoxDecoratedNameIMap m_idecoratedBoxNames;


    static bool hasUnknownInputFormats(const Box *box);
    static BoxList linkBoxes(const LinkList& links);
};

class BoxBreakException
{
public:
    ~BoxBreakException() throw() {}
    explicit BoxBreakException(RuntimeBox *rtbox) throw() : m_rtbox(rtbox) {}
    RuntimeBox *rtbox() const throw() {
        return m_rtbox;
    }
private:
    RuntimeBox *m_rtbox;
};

class EQUARES_CORESHARED_EXPORT Runner : public Runnable
{
public:
    void start(const Simulation *sim);
    void run();
    void requestTermination();
    bool terminationRequested() const;

#ifdef EQUARES_DUMP_SIMULATION_LOG
    void postPortActivation(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg, const QString& itemId);
#else // EQUARES_DUMP_SIMULATION_LOG
    void postPortActivation(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg);
#endif // EQUARES_DUMP_SIMULATION_LOG
    const QVector<RuntimeBox*>& rtboxes() const;

private:
    QList< RuntimeBox::Ptr > m_rtboxes;
    QList< RuntimeLink > m_rtlinks;
    QVector<RuntimeBox*> m_rtboxvec;

    class QueueItem {
    public:
#ifdef EQUARES_DUMP_SIMULATION_LOG
        QueueItem(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg, const QString& itemId) :
            box(box), notifier(notifier), notifierArg(notifierArg), m_itemId(itemId)
#else // EQUARES_DUMP_SIMULATION_LOG
        QueueItem(RuntimeBox *box, RuntimeBox::PortNotifier notifier, int notifierArg) :
            box(box), notifier(notifier), notifierArg(notifierArg)
#endif // EQUARES_DUMP_SIMULATION_LOG
        {
            Q_ASSERT(box);
            Q_ASSERT(notifier);
        }
        bool activate() const {
#ifdef EQUARES_DUMP_SIMULATION_LOG
            int id = equaresNextLogOperationId();
            equaresSimLog() << id << " ACTIVATE_FROM_QUEUE " << m_itemId << endl;
            bool result = (box->*notifier)(notifierArg);
            equaresSimLog() << id << " RESULT " << result << endl;
            return result;
#else // EQUARES_DUMP_SIMULATION_LOG
            return (box->*notifier)(notifierArg);
#endif // EQUARES_DUMP_SIMULATION_LOG
        }

    private:
        RuntimeBox *box;
        RuntimeBox::PortNotifier notifier;
        int notifierArg;
#ifdef EQUARES_DUMP_SIMULATION_LOG
        QString m_itemId;
#endif // EQUARES_DUMP_SIMULATION_LOG
    };

    QList<QueueItem> m_queue;
    QAtomicInt m_terminationRequested;

    void initQueue();
};

inline bool RuntimeInputPort::activate() const {
#ifdef EQUARES_DUMP_SIMULATION_LOG
    int id = equaresNextLogOperationId();
    equaresSimLog() << id << " ACTIVATE " << toString() << endl;
    RuntimeBox::PortNotifier notifier = portNotifier();
    bool result;
    if (notifier) {
        equaresSimLog() << id << " HANDLED" << endl;
        result = (owner()->*notifier)(portId());
    }
    else {
        equaresSimLog() << id << " UNHANDLED" << endl;
        result = true;
    }
    equaresSimLog() << id << " RESULT " << result << endl;
    return result;
#else // EQUARES_DUMP_SIMULATION_LOG
    RuntimeBox::PortNotifier notifier = portNotifier();
    if (!notifier)
        return true;
    return (owner()->*notifier)(portId());
#endif // EQUARES_DUMP_SIMULATION_LOG
}

class EQUARES_CORESHARED_EXPORT BoxFactory
{
public:
    static Box *newBox(const QString& name);
    static Box::Ctor boxCtor(const QString& name);
    static void registerBoxType(const QString& name, Box::Ctor ctor);
    static QStringList boxTypes();
private:
    typedef QMap<QString, Box::Ctor> BoxMap;
    static BoxMap *m_registry;
    static BoxMap& registry();
};

template< class BoxType >
class BoxRegistrator {
public:
    BoxRegistrator(const QString& name) {
        BoxFactory::registerBoxType(name, &BoxRegistrator<BoxType>::newBox);
    }
private:
    static Box *newBox() { return new BoxType(); }
};

#ifdef EQUARES_CORE_STATIC
#define REGISTER_BOX(ClassName, UserName)
#define REGISTER_BOX_STATIC(ClassName, UserName) \
    static BoxRegistrator<ClassName> boxRegistratorFor##ClassName(UserName);
#else // EQUARES_CORE_STATIC
#define REGISTER_BOX(ClassName, UserName) \
    static BoxRegistrator<ClassName> boxRegistratorFor##ClassName(UserName);
#define REGISTER_BOX_STATIC(ClassName, UserName)
#endif // EQUARES_CORE_STATIC

#endif // EQUARES_CORE_H
