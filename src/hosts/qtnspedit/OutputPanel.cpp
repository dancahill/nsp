// OutputPanel.cpp - Implementation of the script output display panel.
// Black background with light text, using Courier New font for monospaced
// script output. Auto-scrolls to the bottom on each append.

#include "OutputPanel.h"

#include <QScrollBar>

OutputPanel::OutputPanel(QWidget *parent)
    : QTextBrowser(parent)
{
    setReadOnly(true);
    setAcceptRichText(false);
    setLineWrapMode(QTextBrowser::NoWrap);

    // Dark terminal-style color scheme
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::black);
    pal.setColor(QPalette::Text, QColor(220, 220, 220));
    setPalette(pal);

    QFont font("Courier New", 10);
    setFont(font);
}

// Appends text and auto-scrolls to the bottom so the latest output is visible.
// Uses insertPlainText instead of append to avoid the extra newline that
// QTextBrowser::append() prepends.
void OutputPanel::appendOutput(const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

// Clears all output text from the panel.
void OutputPanel::clearOutput()
{
    clear();
}