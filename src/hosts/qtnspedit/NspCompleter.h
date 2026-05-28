// NspCompleter.h - IntelliSense-style autocomplete for NSP script editing.
// Provides a custom completion popup (CompletionPopup) with type-colored icons,
// a filter (NspCompletionFilter) that intercepts Tab/Enter/Escape keys, and
// the main controller (NspCompleter) that populates suggestions from NspNameSpace
// and inserts completions into the CodeEditor.

#ifndef NSPCOMPLETER_H
#define NSPCOMPLETER_H

#include <QCompleter>
#include <QStandardItemModel>
#include <QTreeView>
#include <QLabel>
#include "NspNameSpace.h"

class CodeEditor;

// CompletionPopup: A custom QTreeView popup used to display completion
// suggestions. Extends QTreeView with a tooltip-style info label that shows
// type, description, params, and returns for the currently selected item.
class CompletionPopup : public QTreeView {
    Q_OBJECT
public:
    explicit CompletionPopup(QWidget *parent = nullptr);
    // Returns the info label widget positioned next to the popup for hover details.
    QLabel *infoLabel() const { return m_infoLabel; }

protected:
    // Hides the info label when the popup is hidden.
    void hideEvent(QHideEvent *event) override;

private:
    QLabel *m_infoLabel;
};

class NspCompleter;

// NspCompletionFilter: A QCompleter subclass that overrides eventFilter to
// intercept Tab, Enter, and Escape keys before the base QCompleter processes
// them. On Tab/Enter, it reads the stored selected name and calls
// insertCompletion() then hides the popup. On Escape, it just hides the popup.
// Arrow keys still pass through to QCompleter's base class for popup navigation.
class NspCompletionFilter : public QCompleter {
    Q_OBJECT
public:
    explicit NspCompletionFilter(QAbstractItemModel *model, QObject *parent = nullptr)
        : QCompleter(model, parent), m_nspCompleter(nullptr) {}
    // Sets the owning NspCompleter so the filter can delegate insertion.
    void setNspCompleter(NspCompleter *nc) { m_nspCompleter = nc; }
protected:
    // Intercepts KeyPress events for Tab/Enter/Escape when popup is visible.
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    NspCompleter *m_nspCompleter;
};

// NspCompleter: The main autocomplete controller. Populates the completion
// model from NspNameSpace entries, manages the popup lifecycle, and inserts
// completed text into the CodeEditor. Functions get "()" appended, tables
// get "." appended after insertion.
class NspCompleter : public QObject {
    Q_OBJECT
public:
    NspCompleter(CodeEditor *editor, QObject *parent = nullptr);

    // Shows the completion popup after typing a dot, listing members of the
    // namespace preceding the dot.
    void popupForDot();
    // Returns the underlying QCompleter used for filtering.
    QCompleter *completer() const { return m_completer; }
    // Returns the item model holding completion entries (name+signature, description).
    QStandardItemModel *model() const { return m_model; }
    // Returns the custom popup widget for showing completions.
    CompletionPopup *popup() const { return m_popup; }

    // Returns true if the completion popup is currently visible.
    bool isPopupVisible() const;
    // Triggers completion based on the current identifier prefix at cursorPos.
    // Called when the user has typed 2+ identifier characters.
    void triggerCompletion(int cursorPos);
    // Extracts and returns the current word prefix (identifier chars before cursor).
    QString currentWordPrefix() const;
    // Returns the name of the currently selected completion item, stored by
    // onCurrentIndexChanged via Qt::UserRole data.
    QString selectedName() const { return m_selectedName; }

public slots:
    // Inserts the completion text into the editor, replacing the typed prefix.
    // Appends "()" for functions (positioning cursor inside parens) and "." for tables.
    void insertCompletion(const QString &completion);

private slots:
    // Called when the popup selection changes; stores the selected item's name
    // into m_selectedName and displays an info tooltip next to the popup.
    void onCurrentIndexChanged(const QModelIndex &index);

private:
    // Fills m_model with rows for the given entries, filtering by prefix.
    // Each row shows a type-colored icon, name (with params for functions),
    // and a description snippet.
    void populateModel(const QList<NspNameSpace::Entry> &entries, const QString &prefix);

    CodeEditor *m_editor;               // The editor widget being complemented
    NspCompletionFilter *m_completer;  // Filter that intercepts key events
    QStandardItemModel *m_model;        // Model holding completion entries
    CompletionPopup *m_popup;           // Custom tree-view popup widget
    QString m_currentNamespace;          // Namespace path preceding a dot (e.g., "math" for "math.sin")
    int m_completionStartPos;            // Cursor position where the current completion prefix starts
    QString m_selectedName;              // Name of the currently highlighted completion item
};

#endif // NSPCOMPLETER_H