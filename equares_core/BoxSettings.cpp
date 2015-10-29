/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "equares_core.h"
#include <functional>

template<class ElementType>
static void readPtrListSettings(const QList<ElementType*>& lst, QSettings &settings, const QString& name)
{
    int n = settings.beginReadArray(name);
    int index = 0;
    foreach (ElementType *x, lst) {
        if (index >= n)
            break;
        settings.setArrayIndex(index++);
        x->loadSettings(settings);
    }
    settings.endArray();
}

template<class ElementType, class Object, class Setter>
static void readListSettings(Object object, Setter setter, QSettings &settings, const QString& name)
{
    int n = settings.beginReadArray(name);
    QList<ElementType> lst;
    for (int index=0; index<n; ++index) {
        settings.setArrayIndex(index);
        ElementType x;
        x.loadSettings(settings);
        lst << x;
    }
    setter(object, lst);
    settings.endArray();
}



EntryHints& EntryHints::loadSettings(QSettings& settings)
{
    m_entryHints = settings.value("entries").toStringList();
//    m_entryHints.clear();
//    foreach(const QString& hint, settings.value("entries").toString().split(",", QString::SkipEmptyParts))
//        m_entryHints << hint.trimmed();
    return *this;
}

PortHints& PortHints::loadSettings(QSettings& settings)
{
    EntryHints::loadSettings(settings);
    m_position = settings.value("position", -1.0).toDouble();
    return *this;
}

Port& Port::loadSettings(QSettings& settings)
{
    setHelpString(settings.value("help").toString());
    hints().loadSettings(settings);
    return *this;
}

BoxProperty& BoxProperty::loadSettings(QSettings& settings)
{
    name = settings.value("name").toString();
    helpString = settings.value("help").toString();
    userType = settings.value("usertype").toString();
    userType.replace('\'', '"');
    toUserType = settings.value("tousertype").toString();
    toBoxType = settings.value("toboxtype").toString();
    deps = settings.value("deps").toStringList();
    resolveUserType = settings.value("resolveusertype").toString();
    critical = settings.value("critical").toBool();
    snippet = settings.value("snippet").toString();
    return *this;
}

Box& Box::loadSettings(QSettings &settings)
{
    setHelpString(settings.value("help").toString());
    readPtrListSettings(inputPorts(), settings, "inputs");
    readPtrListSettings(outputPorts(), settings, "outputs");
    readListSettings<BoxProperty>(this, std::mem_fun(&Box::setBoxProperties), settings, "properties");
    return *this;
}
