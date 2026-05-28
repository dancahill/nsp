// OutputPanel.h - A read-only QTextBrowser with black background that
// displays script execution output. Appended to by the ScriptRunner via
// the outputReady signal.

#ifndef OUTPUTPANEL_H
#define OUTPUTPANEL_H

#include <QTextBrowser>

class OutputPanel : public QTextBrowser {
    Q_OBJECT
public:
    explicit OutputPanel(QWidget *parent = nullptr);

public slots:
    // Appends the given text to the output panel and scrolls to the bottom.
    void appendOutput(const QString &text);
    // Clears all output text.
    void clearOutput();
};

#endif // OUTPUTPANEL_H