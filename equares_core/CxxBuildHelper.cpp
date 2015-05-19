/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include "CxxBuildHelper.h"
#include "equares_core.h"
#include "PerTypeStorage.h"
#include "check_lib.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegExp>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QLibrary>
#include "box_util.h"

static void throwBoxException(const Box *box, const QString& what) {
    throw EquaresBoxException(box, what);
}

// http://stackoverflow.com/questions/11050977/removing-a-non-empty-folder-in-qt
static bool removeDir(const QString& dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir())
                result = removeDir(info.absoluteFilePath());
            else
                result = QFile::remove(info.absoluteFilePath());
            if (!result)
                return result;
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

class DirRemover
{
public:
    explicit DirRemover(const QString& dirName) : m_dirName(dirName), m_cancel(false) {}
    ~DirRemover() {
        if (!m_cancel)
            removeDir(m_dirName);
    }
    void cancel() {
        m_cancel = true;
    }
private:
    QString m_dirName;
    bool m_cancel;
};

#ifdef _WIN32
#define PLATFORM WINDOWS

static void buildLibNoQmake(const QDir& dir, const Box *box, const QString& libBaseName)
{
    QString buildFileName = "build_msvs.bat";
    QFileInfo fi(dir.absoluteFilePath(buildFileName));
    if (!fi.exists()) {
        if (!QFile::copy(":/cxx/"+buildFileName, fi.absoluteFilePath()))
            throwBoxException(box, QString("Failed to copy library build file to '%1'")
                .arg(fi.absoluteFilePath()));
    }
    QProcess proc;
    QString cmdline = "cmd /C " + buildFileName + " " + libBaseName + ".cpp";
    proc.setWorkingDirectory(dir.absolutePath());
    proc.start(cmdline);
    if (!proc.waitForFinished())
        throwBoxException(box, "Failed to build dll-file (timed out)");
    if (proc.exitCode() != 0)
        throwBoxException(box, QString("Failed to build library file:\n%1").arg(
            QString::fromUtf8(proc.readAllStandardError())));
    Q_ASSERT(QFileInfo(dir.absoluteFilePath(libBaseName + ".dll")).exists());
}

#endif // _WIN32

#ifdef __linux__
#define PLATFORM LINUX

static void buildLibNoQmake(const QDir& dir, const Box *box, const QString& libBaseName)
{
    QString cmdline = QString("g++ -Wall -shared -Wl,-soname,%1.so -fPIC -O2 -I../../../../public/include -o %1.so %1.cpp").arg(libBaseName);
    QProcess proc;
    proc.setWorkingDirectory(dir.absolutePath());
    proc.start(cmdline);
    if (!proc.waitForFinished())
        throwBoxException(box, "Failed to build so-file (timed out)");
    if (proc.exitCode() != 0)
        throwBoxException(box, QString("Failed to build library file:\n%1").arg(
            QString::fromUtf8(proc.readAllStandardError())));
    Q_ASSERT(QFileInfo(dir.absoluteFilePath(libBaseName + ".so")).exists());
}

#endif // __linux__

#ifndef PLATFORM
#error "Unknown platform"
#endif // !PLATFORM

static void buildLibWithQmake(const QDir& dir, const Box *box, const QString& libBaseName)
{
    QFileInfo fiPro(dir.absoluteFilePath(libBaseName + ".pro"));
    if (!fiPro.exists()) {
        if (!QFile::copy(":/cxx/" + libBaseName + ".pro", fiPro.absoluteFilePath()))
            throwBoxException(box, QString("Failed to copy qmake project file to '%1'")
                .arg(fiPro.absoluteFilePath()));
    }
    QProcess proc;
    QStringList env = QProcess::systemEnvironment();
    QDir appDir(QCoreApplication::applicationDirPath());
    QString buildTimePath = readFile(appDir.absoluteFilePath("buildpath.txt"), box);
    buildTimePath.remove('\r').remove('\n');
    QString buildTimeInclude = readFile(appDir.absoluteFilePath("buildinclude.txt"), box);
    buildTimeInclude.remove('\r').remove('\n');
    QString buildTimeLib = readFile(appDir.absoluteFilePath("buildlib.txt"), box);
    buildTimeLib.remove('\r').remove('\n');
    QString makeCmd = readFile(appDir.absoluteFilePath("makecmd.txt"), box);
    makeCmd.remove('\r').remove('\n');
#ifdef _WIN32
    // Because it might happen that makeCmd is not found in the path
    // for this process
    makeCmd.prepend("cmd /C ");
#endif // _WIN32
    QRegExp rx("^(PATH|INCLUDE|LIB)=");
    for (int i=0; i<env.size();) {
        if (rx.indexIn(env[i]) != -1)
            env.removeAt(i);
        else
            ++i;
    }
    env << "PATH="+buildTimePath
        << "INCLUDE="+buildTimeInclude
        << "LIB="+buildTimeLib;
    proc.setWorkingDirectory(dir.absolutePath());
    proc.setEnvironment(env);
    proc.start("qmake " + libBaseName + ".pro");
    if (!proc.waitForFinished())
        throwBoxException(box, "Failed to generate Makefile (timed out)");
    if (proc.exitCode() != 0)
        throwBoxException(box, QString("Failed to generate Makefile (qmake returned %1):\n%2").arg(
            QString::number(proc.exitCode()),
            QString::fromUtf8(proc.readAllStandardError())));

    proc.start(makeCmd);
    if (!proc.waitForFinished())
        throwBoxException(box, "Failed to build library (timed out)");

    if (proc.exitCode() != 0) {
        EQUARES_COUT << proc.readAllStandardOutput() << endl;
        throwBoxException(box, QString("Failed to build library :\n%1").arg(
            QString::fromUtf8(proc.readAllStandardError())));
    }
}

static void buildLib(const QDir& dir, bool withQmake, const Box *box, const QString& libBaseName)
{
    DirRemover drm(dir.absolutePath());
    if (withQmake)
        buildLibWithQmake(dir, box, libBaseName);
    else
        buildLibNoQmake(dir, box, libBaseName);
    drm.cancel();
}



CxxBuildHelper::CxxBuildHelper(
    const Box *box,
    const QString &src,
    const QString &libBaseName,
    const QString &footerFileName,
    bool useQmake
    ) :
    m_box(box),
    m_src(src),
    m_libBaseName(libBaseName),
    m_footerFileName(footerFileName),
    m_useQmake(useQmake),
    m_hasNames(false),
    m_namesExtracted(false)
{
    Q_ASSERT(box);
}

QString CxxBuildHelper::buildDirName() const throw() {
    return extractNames() ?   m_className + "_" + m_hashString :   QString();
}

QString CxxBuildHelper::className() const throw() {
    return extractNames() ?   m_className :   QString();
}

QString CxxBuildHelper::hashString() const throw() {
    return extractNames() ?   m_hashString :   QString();
}

QLibraryPtr CxxBuildHelper::library() const
{
    if (!extractNames())
        throwBoxException(m_error);

    // Generate library subdir path
    QString subdirPath = "equares/" + buildDirName();

    // Library file full path name will be stored here
    QString libName;

    // Library file base name
    QString baseName = m_libBaseName;

    // Obtain global settings instance
    const QVariantMap& globalSettings = PerTypeStorage::instance<QVariantMap>();

    // At first, look up the library in the cache directory
    QString cacheDir = globalSettings["cacheDir"].toString();
    if (!cacheDir.isEmpty()) {
        QDir dir(cacheDir);
        if (dir.exists() && dir.cd(subdirPath) && libUpToDate(dir.absoluteFilePath(baseName), m_hashString))
            libName = dir.absoluteFilePath(baseName);
    }

    if (libName.isEmpty()) {
        // The library is missing in the cache, try looking up in the current dir; build if necessary and allowed
        bool canBuild = !globalSettings["denyBuild"].toBool();

        // Prepare work directory
        QDir dir = QDir::current();
        if (canBuild && !dir.mkpath(subdirPath))
            throwBoxException(QString("Failed to create directory %1").arg(dir.absoluteFilePath(subdirPath)));
        if (!dir.cd(subdirPath))
            throwBoxException(QString("Failed to switch to directory %1").arg(dir.absoluteFilePath(subdirPath)));

        if (!libUpToDate(dir.absoluteFilePath(baseName), m_hashString)) {
            if (!canBuild)
                throwBoxException("Failed to build library because build is denied");
            // Generate source file
            QFileInfo fi(dir.absoluteFilePath(baseName + ".cpp"));
            {
                QString srcFileContent =
                    m_src + "\n" +
                    readFile(m_footerFileName, m_box).replace("<X>", m_className) + "\n" +
                        "extern \"C\" EQUARES_LIB_EXPORT const char *hash() { return \"" + m_hashString + "\"; }\n";
                QFile srcFile(fi.absoluteFilePath());
                if (!srcFile.open(QIODevice::WriteOnly))
                    throwBoxException(QString("Failed to open file %1").arg(fi.absoluteFilePath()));
                srcFile.write(srcFileContent.toUtf8());
            }

            // Build library
            buildLib(dir, m_useQmake, m_box, m_libBaseName);
        }
        libName = dir.absoluteFilePath(baseName);
    }

    // Check library before loading it
    checkLib(libName);

    // Load library
    QLibraryPtr lib(new QLibrary(libName));
    if (!lib->load())
        throwBoxException(QString("Failed to load library %1").arg(libName));

    // Return result
    return lib;
}

bool CxxBuildHelper::extractNames() const
{
    if (m_namesExtracted)
        return m_hasNames;

    m_namesExtracted = true;
    Q_ASSERT(!m_hasNames);

    // Extract class name that will also be part of directory name
    {
        QRegExp rx("^\\s*struct\\s+([a-zA-Z_][a-zA-Z0-9_]*)");
        foreach (const QString& line, m_src.split('\n'))
            if (rx.indexIn(line) != -1) {
                m_className = rx.capturedTexts()[1];
                Q_ASSERT(!m_className.isEmpty());
                break;
            }
        if (m_className.isEmpty()) {
            m_error = "Failed to retrieve struct name from source code";
            return false;
        }
    }

    // Generate md5 checksum
    {
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(m_src.toUtf8());
        const char *xdigits = "0123456789abcdef";
        foreach(unsigned char c, hash.result()) {
            m_hashString += xdigits[(c>>4) & 0xf];
            m_hashString += xdigits[c & 0xf];
        }
    }

    return m_hasNames = true;
}

bool CxxBuildHelper::libUpToDate(const QString &libName, const QString& hashString) const
{
    try {
        QFileInfo fi(libName);
        QString libFileName = fi.dir().absoluteFilePath("lib" + fi.baseName() + ".so");
        if (QFileInfo(libFileName).exists()) {

            // Check library before loading it
            checkLib(libName);

            // Load library
            QLibrary lib(libName);
            if (!lib.load())
                throwBoxException(QString("Failed to load library %1").arg(libName));

            // Check hash
            typedef const char* (*HashFunc)();
            HashFunc hashFunc = reinterpret_cast<HashFunc>(lib.resolve("hash"));
            if (!hashFunc)
                throwBoxException(QString("Failed to resolve symbol 'hash' in library '%1'").arg(libName));
            return hashFunc() == hashString;
        }
        else
            return false;
    }
    catch (const EquaresException&) {
        return false;
    }
}

void CxxBuildHelper::checkLib(const QString& libName) const
{
    if (!extractNames())
        throwBoxException(QString("Failed to extract names from library source code"));
    try {
        ::checkLib(libName, m_className);
    }
    catch(const std::exception& e) {
        throwBoxException(QString("Security check has failed on library file '%1':\n%2").arg(libName, QString::fromUtf8(e.what())));
    }
}

void CxxBuildHelper::throwBoxException(const QString& what) const {
    ::throwBoxException(m_box, what);
}
