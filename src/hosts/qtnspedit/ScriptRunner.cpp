// ScriptRunner.cpp - Implementation of the NSP script execution thread.
// Creates an NSP interpreter state, populates the _ENV table with system
// environment variables, registers flush/break callbacks, and runs the
// script. Emits signals for output, breakpoints, and completion.

#include <QProcess>
#include <QFileInfo>
#include <QDir>

// Qt includes must come before nsp.h to avoid qhash.h conflict
#include "ScriptRunner.h"

// The currently running ScriptRunner instance, accessed by static C callbacks.
ScriptRunner *ScriptRunner::s_currentRunner = nullptr;

ScriptRunner::ScriptRunner(const QString &script, const QString &filename, QObject *parent)
    : QThread(parent)
    , m_script(script)
    , m_filename(filename)
    , m_nsp(nullptr)
{
}

// Destructor: waits for the thread to finish before destroying the object.
ScriptRunner::~ScriptRunner()
{
    wait();
}

// ScriptRunner::run
// The main thread entry point. Creates an NSP interpreter state, sets up
// environment variables, registers custom flush and breakpoint callbacks,
// executes the script, and emits completion signals with memory statistics.
void ScriptRunner::run()
{
    s_currentRunner = this;
    m_nsp = nsp_newstate();
    if (!m_nsp) {
        emit scriptFinished(true, "Failed to create NSP state");
        s_currentRunner = nullptr;
        return;
    }

    // Populate the NSP _ENV table with system environment variables
    // so scripts can access them via _ENV.VARNAME
    obj_t *envTable = nsp_settable(m_nsp, &m_nsp->g, (char *)"_ENV");
    const QStringList envList = QProcess::systemEnvironment();
    for (const QString &var : envList) {
        int sep = var.indexOf('=');
        if (sep > 0) {
            QByteArray key = var.left(sep).toUtf8();
            QByteArray val = var.mid(sep + 1).toUtf8();
            nsp_setstr(m_nsp, envTable, key.data(), val.data(), val.length());
        }
    }

    // Set _filename and _filepath global variables so scripts know their own path
    if (!m_filename.isEmpty()) {
        QFileInfo fi(m_filename);
        QByteArray fn = fi.fileName().toUtf8();
        QByteArray fp = fi.absolutePath().toUtf8();
        nsp_setstr(m_nsp, &m_nsp->g, (char *)"_filename", fn.data(), fn.length());
        nsp_setstr(m_nsp, &m_nsp->g, (char *)"_filepath", fp.data(), fp.length());
    }

    // Register lib.io.flush to use our Qt-aware flush callback so output
    // appears in the output panel in real-time
    obj_t *libIo = nsp_settable(m_nsp, nsp_settable(m_nsp, &m_nsp->g, (char *)"lib"), (char *)"io");
    nsp_setcfunc(m_nsp, libIo, (char *)"flush", nspQtFlush);

    // Register lib.debug.break2 to use our breakpoint callback so
    // debug.break() in scripts triggers the breakpointHit signal
    obj_t *libDebug = nsp_settable(m_nsp, nsp_settable(m_nsp, &m_nsp->g, (char *)"lib"), (char *)"debug");
    nsp_setcfunc(m_nsp, libDebug, (char *)"break2", nspQtBreak);

    // Execute the script
    QByteArray scriptData = m_script.toUtf8();
    nsp_exec(m_nsp, scriptData.constData());

    // Emit completion signal with error info if NSP reported an error
    if (m_nsp->err) {
        emit scriptFinished(true, QString::fromUtf8(m_nsp->errbuf));
    } else {
        emit scriptFinished(false, QString());
    }

    // Output memory allocation statistics
    QString stats = QString("\nallocs: %1  frees: %2  peak: %3 bytes\n")
        .arg(m_nsp->allocs).arg(m_nsp->frees).arg(m_nsp->peakmem);
    emit outputReady(stats);

    QString loadtime = QString("Loaded %1 (%2 seconds)\n")
        .arg(m_filename.isEmpty() ? QString("script") : m_filename)
        .arg(0.0);
    emit outputReady(loadtime);

    // Clean up the NSP interpreter state
    nsp_freestate(m_nsp);
    nsp_endstate(m_nsp);
    m_nsp = nullptr;
    s_currentRunner = nullptr;
}

// ScriptRunner::resume
// Called from the main thread to resume script execution after a breakpoint.
// Wakes the thread that is blocked in nspQtBreak().
void ScriptRunner::resume()
{
    m_breakCondition.wakeAll();
}

// ScriptRunner::stop
// Terminates the script thread immediately.
void ScriptRunner::stop()
{
    if (!isRunning()) return;

    // Wake the thread if it's blocked at a breakpoint so it can exit
    m_breakCondition.wakeAll();

    terminate();
    wait(1000);

    m_nsp = nullptr;
}

// ScriptRunner::nspQtFlush
// Static callback registered as lib.io.flush in the NSP interpreter.
// Reads the NSP output buffer and emits the outputReady signal so
// the text appears in the OutputPanel.
int ScriptRunner::nspQtFlush(nsp_state *N)
{
    ScriptRunner *runner = s_currentRunner;
    if (runner && N->outbuflen > 0) {
        QString text = QString::fromUtf8(N->outbuffer, N->outbuflen);
        emit runner->outputReady(text);
        N->outbuflen = 0;
    }
    return 0;
}

// ScriptRunner::nspQtBreak
// Static callback registered as lib.debug.break2 in the NSP interpreter.
// Called when debug.break() is invoked in a script. Emits the breakpointHit
// signal and blocks the script thread until resume() is called from the
// main thread (via Continue action).
int ScriptRunner::nspQtBreak(nsp_state *N)
{
    ScriptRunner *runner = s_currentRunner;
    if (runner) {
        // Notify the main thread that a breakpoint was hit
        emit runner->breakpointHit();
        // Block this thread until the user resumes execution
        QMutexLocker locker(&runner->m_breakMutex);
        runner->m_breakCondition.wait(&runner->m_breakMutex);
    }
    return 0;
}