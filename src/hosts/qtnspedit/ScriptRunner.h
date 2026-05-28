// ScriptRunner.h - QThread subclass that executes NSP scripts in a background
// thread. Sets up the NSP environment, registers output flush and breakpoint
// callbacks, and provides signals for output, completion, and breakpoint hits.

#ifndef SCRIPTRUNNER_H
#define SCRIPTRUNNER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

extern "C" {
#include "nsp/nsp.h"
}

class ScriptRunner : public QThread {
    Q_OBJECT
public:
    // Constructs a runner for the given script content and filename.
    ScriptRunner(const QString &script, const QString &filename, QObject *parent = nullptr);
    // Waits for the thread to finish before destruction.
    ~ScriptRunner();

    // Resumes execution after a breakpoint hit (debug.break() in script).
    void resume();
    // Returns the NSP state pointer for memory inspection (used by MemTreeView).
    nsp_state *nspState() const { return m_nsp; }
    // Returns the filename of the running script.
    QString filename() const { return m_filename; }

signals:
    // Emitted when the script produces output text (via nspQtFlush callback).
    void outputReady(const QString &text);
    // Emitted when the script finishes. error=true if NSP reported an error.
    void scriptFinished(bool error, const QString &errbuf);
    // Emitted when debug.break() is called in the script, pausing execution.
    void breakpointHit();

protected:
    // The main thread entry point: sets up NSP state and runs nsp_exec.
    void run() override;

private:
    QString m_script;       // The script source code to execute
    QString m_filename;     // The filename (used for error messages and _filename var)
    nsp_state *m_nsp;      // The NSP interpreter state
    QMutex m_breakMutex;             // Mutex for the breakpoint wait condition
    QWaitCondition m_breakCondition; // Condition variable to pause/resume at breakpoints

    // Pointer to the currently executing runner, used by static C callbacks.
    static ScriptRunner *s_currentRunner;
    // NSP callback: flushes output buffer and emits outputReady signal.
    static int nspQtFlush(nsp_state *N);
    // NSP callback: pauses execution at a breakpoint and emits breakpointHit signal.
    // Blocks until resume() is called.
    static int nspQtBreak(nsp_state *N);
};

#endif // SCRIPTRUNNER_H