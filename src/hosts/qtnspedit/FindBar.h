// FindBar.h - Inline search overlay for the CodeEditor. Shown via Ctrl+F,
// pre-filled with selected text. Enter/F3 finds next, Shift+F3 finds previous,
// Escape closes the bar and returns focus to the editor.

#ifndef FINDBAR_H
#define FINDBAR_H

#include <QFrame>

class QLineEdit;
class QPlainTextEdit;

class FindBar : public QFrame {
    Q_OBJECT
public:
    // Constructs the find bar as a child of the given editor widget.
    explicit FindBar(QPlainTextEdit *editor, QWidget *parent = nullptr);

    // Shows the bar, pre-fills with the current selection, and focuses the input.
    void showFind();
    // Searches forward from the current cursor position, wrapping around if needed.
    void findNext();
    // Searches backward from the current cursor position, wrapping around if needed.
    void findPrev();

protected:
    // Handles Escape to close the bar and return focus to the editor.
    void keyPressEvent(QKeyEvent *event) override;
    // Focuses the search input when the bar becomes visible.
    void showEvent(QShowEvent *event) override;

private slots:
    // Called when the search text changes; triggers incremental find from cursor.
    void onTextChanged(const QString &text);

private:
    QPlainTextEdit *m_editor;  // The editor to search within
    QLineEdit *m_searchEdit;   // The search input field
};

#endif // FINDBAR_H