#include "DumpBox.h"

REGISTER_BOX(DumpBox, "Dump")

DumpBox::DumpBox(QObject *parent) :
    Box(parent),
    m_dump("input", this)
{
}

InputPorts DumpBox::inputPorts() const {
    return InputPorts() << &m_dump;
}

OutputPorts DumpBox::outputPorts() const {
    return OutputPorts();
}

void DumpBox::checkPortFormat() const
{
    if (!m_dump.format().isValid())
        throwBoxException("DumpBox: no format is specified for port 'dump'");
}

bool DumpBox::propagatePortFormat() {
    return false;
}

RuntimeBox *DumpBox::newRuntimeBox() const {
    return new DumpRuntimeBox(this);
}

QString DumpBox::fileName() const {
    return m_fileName;
}

DumpBox &DumpBox::setFileName(const QString& fileName) {
    m_fileName = fileName;
    return *this;
}



DumpRuntimeBox::DumpRuntimeBox(const DumpBox *box)
{
    setOwner(box);

    InputPorts in = box->inputPorts();
    m_dump.init(this, in[0], toPortNotifier(&DumpRuntimeBox::dump));
    setInputPorts(RuntimeInputPorts() << &m_dump);
    if (box->fileName().isEmpty())
        throwBoxException(QString("DumpRuntimeBox: Output file name is not specified (parameter fileName)"));
    else {
        m_cfile = fopen(box->fileName().toUtf8().data(), "w");
        if(!m_cfile)
            throwBoxException(QString("DumpRuntimeBox: Failed to open output file '%1'").arg(box->fileName()));
    }
    m_totalDataWritten = 0;
}

DumpRuntimeBox::~DumpRuntimeBox()
{
    if (m_cfile && m_cfile != stdout)
        fclose(m_cfile);
}

OutputFileInfoList DumpRuntimeBox::outputFileInfo() const
{
    return OutputFileInfoList() << OutputFileInfo::text(fileName());
}

bool DumpRuntimeBox::dump()
{
    RuntimeOutputPort
        *dumpPort = m_dump.outputPort();
    if (!dumpPort->state().hasData())
        return false;

    int n = dumpPort->port()->format().dataSize();
    if (n == 0)
        return true;
    if (m_totalDataWritten >= DataLimit)
        return true;
    int m = dumpPort->port()->format().size(0);
    Q_ASSERT(m > 0);
    const double *data = dumpPort->data().data();
    for (int i=0; i<n; ++i) {
        int ii = i % m;
        if (ii > 0)
            fputc('\t', m_cfile);
        fprintf(m_cfile, "%.16lg", data[i]);
        ++m_totalDataWritten;
        if (ii+1 == m)
            fputc('\n', m_cfile);
        if (m_totalDataWritten >= DataLimit) {
            fprintf(m_cfile, "\n\n... (data size limit is exceeded)\n");
            fclose(m_cfile);
            m_cfile = 0;
            ThreadManager::instance()->reportProgress(ProgressInfo().setSync(true) << fileName());
            return true;
        }
    }
    // TODO: decimate progress reports
//    fflush(m_cfile);
//    ThreadManager::instance()->reportProgress(ProgressInfo().setSync(true) << fileName());
    return true;
}

QString DumpRuntimeBox::fileName() const
{
    const DumpBox *box = qobject_cast<const DumpBox*>(owner());
    Q_ASSERT(box);
    return box->fileName();
}
