#ifndef JOINBOX_H
#define JOINBOX_H

#include "equares_core.h"

class EQUARES_CORESHARED_EXPORT JoinBox : public Box
{
    Q_OBJECT
public:
    explicit JoinBox(QObject *parent = 0);

    InputPorts inputPorts() const;
    OutputPorts outputPorts() const;
    void checkPortFormat() const;
    bool propagatePortFormat();

    RuntimeBox *newRuntimeBox() const;

private:
    mutable InputPort m_in1;
    mutable InputPort m_in2;
    mutable OutputPort m_out;
};

class EQUARES_CORESHARED_EXPORT JoinRuntimeBox : public RuntimeBox
{
public:
    explicit JoinRuntimeBox(const JoinBox *box);

private:
    RuntimeInputPort m_in[2];
    RuntimeOutputPort m_out;

    bool m_hasData[2];
    int m_availInputs;
    int m_idx[3];
    QVector<double> m_data;

    bool setInput(int portId);
};

#endif // JOINBOX_H
