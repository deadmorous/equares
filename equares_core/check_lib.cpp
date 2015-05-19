/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

// check_lib.cpp

#include "check_lib.h"
#include "EquaresException.h"
#include "equares_exec.h"   // deBUG
#include <QProcess>
#include <QStringList>
#include <QFileInfo>
#include <QRegExp>
#include <QDir>
#include <QSet>

static QString readFile(const QString& fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen())
        throw EquaresException(QObject::tr("readFile(): failed to open file %1").arg(fileName));
    return QString::fromUtf8(file.readAll());
}

void checkLib(const QString& libName, const QString &userClassName)
{
    static QString script;
    static QSet<QString> whiteList;
    static QRegExp rx("^\\w+[^#]*");
    if (script.isEmpty()) {
        // Read soimports command and whitelist
        script = readFile(":/checklib/soimports").split('\n')[0];
        foreach (const QString& s, readFile(":/checklib/soimports-whitelist.txt").split('\n'))
            if (rx.indexIn(s) == 0)
                whiteList << rx.capturedTexts()[0];
    }

    QProcess proc;
    QFileInfo fi(libName);
    QString libFileName = fi.dir().absoluteFilePath("lib" + fi.baseName() + ".so");
    QString cmd = script;
    cmd.replace("$1", libFileName);
    proc.start("sh");
    if (!proc.waitForStarted())
        throw EquaresException(QObject::tr("Failed to start command\n%1").arg(cmd));
    proc.write(cmd.toUtf8());
    proc.closeWriteChannel();
    if (!proc.waitForFinished())
        throw EquaresException(QObject::tr("Command\n%1\ntakes too long to execute").arg(cmd));
    QString procStderr = QString::fromUtf8(proc.readAllStandardError());
    if (proc.exitStatus() != QProcess::NormalExit   ||   proc.exitCode() != 0   ||   !procStderr.isEmpty())
        throw EquaresException(QObject::tr("Command\n%1\nfailed: code: %2, message: %3")
            .arg(cmd, QString::number(proc.exitCode()), procStderr));
    QString procStdout = QString::fromUtf8(proc.readAllStandardOutput());
    if (procStdout.isEmpty())
        throw EquaresException(QObject::tr("Command\n%1\nreturned nothing").arg(cmd));
    QSet<QString> prohibitedSymbols;
    QString userClassNamePrefix = userClassName + "::";
    foreach (const QString& line, procStdout.split('\n')) {
        if (line.isEmpty())
            continue;
        if (rx.indexIn(line) != 0)
            throw EquaresException(QObject::tr("Command\n%1\nfailed: unrecognized output").arg(cmd));
        QString s = rx.capturedTexts()[0];
#ifdef QT_DEBUG
        if (s.contains("::"))
            continue;   // Skip c++ symbols (TODO better)
#endif // QT_DEBUG
        if (s.contains(userClassNamePrefix))
            continue;   // Skip methods of user's class
        if (!whiteList.contains(s))
            prohibitedSymbols << s;
    }
    if (!prohibitedSymbols.isEmpty())
        throw EquaresException(QObject::tr("The following imported or weak symbols are not allowed:\n%1")
            .arg(QStringList(prohibitedSymbols.toList()).join(", ")));
}
