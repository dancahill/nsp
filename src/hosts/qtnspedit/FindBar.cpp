// FindBar.cpp - Implementation of the inline find bar overlay for CodeEditor.
// Provides forward/backward search with wrap-around, pre-filling from selection,
// and Escape to close.

#include "FindBar.h"

#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QKeyEvent>
#include <QStyle>

// FindBar constructor
// Creates a horizontal bar with a search input, previous/next buttons,
// and a close button. Initially hidden; shown by showFind().
FindBar::FindBar(QPlainTextEdit *editor, QWidget *parent)
    : QFrame(parent)
    , m_editor(editor)
{
    setFrameShape(StyledPanel);
    setAutoFillBackground(true);
    setFixedHeight(32);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Find...");
    m_searchEdit->setMaximumWidth(300);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(24);

    // Previous button: searches backward (Shift+F3)
    QToolButton *prevBtn = new QToolButton(this);
    prevBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    prevBtn->setToolTip("Find Previous (Shift+F3)");
    prevBtn->setAutoRaise(true);
    prevBtn->setFixedSize(24, 24);

    // Next button: searches forward (F3)
    QToolButton *nextBtn = new QToolButton(this);
    nextBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    nextBtn->setToolTip("Find Next (F3)");
    nextBtn->setAutoRaise(true);
    nextBtn->setFixedSize(24, 24);

    // Close button: hides the bar (Escape)
    QToolButton *closeBtn = new QToolButton(this);
    closeBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    closeBtn->setToolTip("Close (Escape)");
    closeBtn->setAutoRaise(true);
    closeBtn->setFixedSize(24, 24);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_searchEdit);
    layout->addWidget(prevBtn);
    layout->addWidget(nextBtn);
    layout->addStretch();
    layout->addWidget(closeBtn);

    // Enter in search field triggers find-next
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindBar::findNext);
    // Text changes trigger incremental search
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FindBar::onTextChanged);
    connect(nextBtn, &QToolButton::clicked, this, &FindBar::findNext);
    connect(prevBtn, &QToolButton::clicked, this, &FindBar::findPrev);
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::hide);

    hide();
}

// FindBar::showFind
// Shows the bar, pre-fills with selected text if any, and focuses the input.
void FindBar::showFind()
{
    show();
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection())
        m_searchEdit->setText(cursor.selectedText());
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

// FindBar::findNext
// Searches forward from the current cursor position. Wraps around to the
// beginning of the document if no match is found after the cursor.
void FindBar::findNext()
{
    QString searchStr = m_searchEdit->text();
    if (searchStr.isEmpty()) return;

    QTextCursor cursor = m_editor->textCursor();
    QTextCursor found = m_editor->document()->find(searchStr, cursor);
    if (!found.isNull()) {
        m_editor->setTextCursor(found);
    } else {
        // Wrap around: search from the beginning of the document
        found = m_editor->document()->find(searchStr, 0);
        if (!found.isNull())
            m_editor->setTextCursor(found);
    }
}

// FindBar::findPrev
// Searches backward from the current cursor position. Wraps around to the
// end of the document if no match is found before the cursor.
void FindBar::findPrev()
{
    QString searchStr = m_searchEdit->text();
    if (searchStr.isEmpty()) return;

    QTextCursor cursor = m_editor->textCursor();
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    QTextCursor found = m_editor->document()->find(searchStr, cursor, flags);
    if (!found.isNull()) {
        m_editor->setTextCursor(found);
    } else {
        // Wrap around: search from the end of the document
        found = m_editor->document()->find(searchStr, m_editor->document()->characterCount() - 1, flags);
        if (!found.isNull())
            m_editor->setTextCursor(found);
    }
}

// FindBar::onTextChanged
// Called when the search text changes; saves cursor position and triggers
// an incremental find-next to highlight the current match.
void FindBar::onTextChanged(const QString &)
{
    QTextCursor savedCursor = m_editor->textCursor();
    savedCursor.setPosition(qMin(savedCursor.selectionStart(), savedCursor.selectionEnd()));
    findNext();
}

// FindBar::keyPressEvent
// Handles Escape to close the bar and return keyboard focus to the editor.
void FindBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        m_editor->setFocus();
        return;
    }
    QFrame::keyPressEvent(event);
}

// FindBar::showEvent
// Focuses the search input when the bar becomes visible.
void FindBar::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    m_searchEdit->setFocus();
}