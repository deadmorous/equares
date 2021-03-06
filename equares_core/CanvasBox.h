/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef CANVASBOX_H
#define CANVASBOX_H

#include "equares_core.h"
#include "equares_script.h"
#include <cmath>
#include <QTime>

struct CanvasBoxDimParam
{
    double vmin;
    double vmax;
    int resolution;
    CanvasBoxDimParam() : vmin(0), vmax(1), resolution(100) {}
    CanvasBoxDimParam(double vmin, double vmax, int resolution) :
        vmin(vmin), vmax(vmax), resolution(resolution) {}
    int index(double d) const {
        Q_ASSERT(resolution >= 0);
        double t = (d - vmin)/(vmax-vmin);
        if (t < 0   ||   t > 1)
            return -1;
        int result = floor(t*resolution);
        if (result >= resolution)
            result = resolution-1;
        else {
            Q_ASSERT(result >= 0);
        }
        return result;
    }
};

Q_DECLARE_METATYPE(CanvasBoxDimParam)

struct CanvasBoxParam
{
public:
    CanvasBoxParam() {}
    CanvasBoxParam(const CanvasBoxDimParam& d1, const CanvasBoxDimParam& d2) {
        m_dimParam[0] = d1;
        m_dimParam[1] = d2;
    }

    CanvasBoxDimParam& operator[](int i) {
        Q_ASSERT(i==0 || i==1);
        return m_dimParam[i];
    }
    const CanvasBoxDimParam& operator[](int i) const {
        return (*const_cast<CanvasBoxParam*>(this))[i];
    }
    int index(const double *d) const {
        int i1 = m_dimParam[0].index(d[0]),
            i2 = m_dimParam[1].index(d[1]);
        if (i1 < 0   ||   i2 < 0)
            return -1;
        return i1 + i2*m_dimParam[0].resolution;
    }

private:
    CanvasBoxDimParam m_dimParam[2];
};

Q_DECLARE_METATYPE(CanvasBoxParam)

class EQUARES_CORESHARED_EXPORT CanvasBox : public Box
{
    Q_OBJECT
    Q_PROPERTY(CanvasBoxParam param READ param WRITE setParam)
    Q_PROPERTY(int refreshInterval READ refreshInterval WRITE setRefreshInterval)
    Q_PROPERTY(int timeCheckCount READ timeCheckCount WRITE setTimeCheckCount)
    Q_PROPERTY(bool clearOnRestart READ clearOnRestart WRITE setClearOnRestart)
    Q_PROPERTY(bool withInputValue READ withInputValue WRITE setWithInputValue)
public:
    explicit CanvasBox(QObject *parent = 0);

    InputPorts inputPorts() const;
    OutputPorts outputPorts() const;
    void checkPortFormat() const;
    bool propagatePortFormat();

    RuntimeBox *newRuntimeBox() const;

    typedef CanvasBoxParam Param;

    Param param() const;
    CanvasBox& setParam(const Param& param);
    int refreshInterval() const;
    CanvasBox& setRefreshInterval(int refreshInterval);
    int timeCheckCount() const;
    CanvasBox& setTimeCheckCount(int timeCheckCount);
    bool clearOnRestart() const;
    CanvasBox& setClearOnRestart(bool clearOnRestart);
    bool withInputValue() const;
    CanvasBox& setWithInputValue(bool withInputValue);

private:
    enum { ResolutionLimit = 5000 };
    Param m_param;
    int m_refreshInterval;      // Refresh interval, in milliseconds
    int m_timeCheckCount;
    bool m_clearOnRestart;
    bool m_withInputValue;
    mutable InputPort m_in;
    mutable InputPort m_flush;
    mutable InputPort m_clear;
    mutable InputPort m_range;
    mutable OutputPort m_out;
};

class EQUARES_CORESHARED_EXPORT CanvasRuntimeBox : public RuntimeBox
{
public:
    explicit CanvasRuntimeBox(const CanvasBox *box);
    void reset();
    void restart();

private:
    RuntimeInputPort m_in;
    RuntimeInputPort m_flush;
    RuntimeInputPort m_clear;
    RuntimeInputPort m_range;
    RuntimeOutputPort m_out;

    CanvasBox::Param m_param;
    int m_refreshInterval;      // Refresh interval, in milliseconds
    int m_timeCheckCount;
    bool m_clearOnRestart;

    QVector<double> m_data;
    QTime m_time;
    int m_timeCheckCounter;
    bool m_rangeValid;

    bool processInputNoValue(int);
    bool processInputWithValue(int);
    bool flush(int);
    bool clear(int);
    bool setRange(int);
};

#endif // CANVASBOX_H
