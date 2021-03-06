/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "OdeJsBox.h"
#include "equares_script.h"

REGISTER_BOX(OdeJsBox, "JsOde")

OdeJsBox::OdeJsBox(QObject *parent) :
    Box(parent),
    m_param("parameters", this, PortFormat(0).setFixed()),
    m_state("state", this, PortFormat(0).setFixed()),
    m_rhs("oderhs", this, PortFormat(0).setFixed())
{
}

InputPorts OdeJsBox::inputPorts() const {
    return InputPorts() << &m_param << &m_state;
}

OutputPorts OdeJsBox::outputPorts() const {
    return OutputPorts() << &m_rhs;
}

void OdeJsBox::checkPortFormat() const {
    if (m_param.format() != PortFormat(paramCount()))
        throwBoxException("OdeJsBox: port 'parameters' has an invalid size");
    if (m_state.format() != PortFormat(varCount()+1))
        throwBoxException("OdeJsBox: port 'state' has an invalid size");
    if (m_rhs.format() != PortFormat(varCount()))
        throwBoxException("OdeJsBox: port 'rhs' has an invalid size");
}

bool OdeJsBox::propagatePortFormat() {
    return false;
}

RuntimeBox *OdeJsBox::newRuntimeBox() const {
    return new OdeJsRuntimeBox(this);
}

QScriptValue OdeJsBox::ode() const {
    return m_ode;
}

OdeJsBox& OdeJsBox::setOde(const QScriptValue& ode) {
    m_ode = ode;
    m_param.format().setSize(paramCount());
    m_state.format().setSize(varCount()+1);
    m_rhs.format().setSize(varCount());
    return *this;
}

int OdeJsBox::paramCount() const {
    if (!m_ode.isObject())
        throwBoxException("OdeJsBox: property 'ode' must be an object");
    QScriptValue n = m_ode.property("paramCount");
    if (!n.isNumber())
        throwBoxException("OdeJsBox: property 'ode.paramCount' must be a number");
    return n.toInt32();
}

int OdeJsBox::varCount() const {
    if (!m_ode.isObject())
        throwBoxException("OdeJsBox: property 'ode' must be an object");
    QScriptValue n = m_ode.property("varCount");
    if (!n.isNumber())
        throwBoxException("OdeJsBox: property 'ode.varCount' must be a number");
    return n.toInt32();
}



OdeJsRuntimeBox::OdeJsRuntimeBox(const OdeJsBox *box)
{
    setOwner(box);

    InputPorts in = box->inputPorts();
    m_param.init(this, in[0], toPortNotifier(&OdeJsRuntimeBox::setParameters));
    m_state.init(this, in[1], toPortNotifier(&OdeJsRuntimeBox::setState));
    setInputPorts(RuntimeInputPorts() << &m_param << &m_state);

    OutputPorts out = box->outputPorts();
    m_rhsData.resize(out[0]->format().dataSize());
    m_rhs.init(this, out[0], PortData(2, m_rhsData.data()));
    setOutputPorts(RuntimeOutputPorts() << &m_rhs);

    m_hasParamData = false;
    m_paramData.resize(in[0]->format().dataSize());
    m_stateData.resize(in[1]->format().dataSize());

    m_ode = box->ode();

    // Note: don't use box->engine(): it's 0 because we're not running the box
    m_engine = box->simulation()->engine();

    m_rhsFunc = m_ode.property("rhs");
    if (!m_rhsFunc.isFunction())
        throwBoxException("OdeJsBox: property 'ode.rhs' must be a function");
    m_argsForRhs = m_engine->newArray(3);

    m_prepareFunc = m_ode.property("prepare");
    if (m_prepareFunc.isUndefined())
        m_hasPrepareFunc = false;
    else {
        if (!m_prepareFunc.isFunction())
            throwBoxException("OdeJsBox: property 'ode.prepare' must be a function if it exists");
        m_hasPrepareFunc = true;
        m_argsForPrepare = m_engine->newArray(1);
    }
}

bool OdeJsRuntimeBox::setParameters(int) {
    RuntimeOutputPort *paramPort = m_param.outputPort();
    Q_ASSERT(paramPort->state().hasData());
    paramPort->data().copyTo(m_paramData.data());
    m_hasParamData = true;
    if (m_hasPrepareFunc) {
        QScriptValue paramArg = CustomDoubleArrayClass::toScriptValue(m_engine, m_paramData);
        m_argsForPrepare.setProperty(0, paramArg);
        m_prepareFunc.call(m_ode, m_argsForPrepare);
    }
    return true;
}

bool OdeJsRuntimeBox::setState(int) {
    if (!m_hasParamData)
        return false;
    RuntimeOutputPort *statePort = m_state.outputPort();
    Q_ASSERT(statePort->state().hasData());
    if (m_stateData.data() != statePort->data().data())
        statePort->data().copyTo(m_stateData.data());
    QScriptValue paramArg = CustomDoubleArrayClass::toScriptValue(m_engine, m_paramData);
    QScriptValue stateArg = CustomDoubleArrayClass::toScriptValue(m_engine, m_stateData);
    QScriptValue rhsArg = CustomDoubleArrayClass::toScriptValue(m_engine, m_rhsData);
    m_argsForRhs.setProperty(0, rhsArg);
    m_argsForRhs.setProperty(1, paramArg);
    m_argsForRhs.setProperty(2, stateArg);
    m_rhsFunc.call(m_ode, m_argsForRhs);
    m_rhsData = qscriptvalue_cast< QVector<double> >(rhsArg);
    m_rhs.data().copyFrom(m_rhsData.data());
    m_rhs.state().setValid();
    return m_rhs.activateLinks();
}
