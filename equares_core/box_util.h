/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef BOX_UTIL_H
#define BOX_UTIL_H

#include "equares_core.h"

bool propagateCommonFormat(Port& port1, Port& port2);
bool propagateCommonFormat(const QList<Port*>& ports);
bool specifyFormat(PortFormat& dst, const PortFormat& src);
QString readFile(const QString& fileName, const Box *box);
QStringList toNameList(const char *s);

template< class X >
class NameMapper {
public:
    typedef X value_type;
    struct Item { X x; const char *name; };
    NameMapper(const Item *items) {
        for (; items->name; ++items) {
            X x = items->x;
            QString name = QString::fromLatin1(items->name);
            Q_ASSERT(!m_x2n.contains(x));
            m_x2n[x] = name;
            Q_ASSERT(!m_n2x.contains(name));
            m_n2x[name] = x;
        }
    }
    QString toName(X x) const {
        return m_x2n.value(x);
    }
    X fromName(const QString& name) const {
        return m_n2x.value(name, static_cast<X>(-1));
    }

private:
    QMap<X, QString> m_x2n;
    QMap<QString, X> m_n2x;
};

#endif // BOX_UTIL_H
