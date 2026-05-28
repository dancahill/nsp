// NspCompleter.cpp - Implementation of IntelliSense-style autocomplete for
// the NSP script editor. Handles popup display, key event filtering, model
// population with type-colored icons, and completion insertion.

#include "NspCompleter.h"
#include "CodeEditor.h"
#include "NspNameSpace.h"

#include <QTextCursor>
#include <QTextBlock>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QHeaderView>
#include <QApplication>
#include <QScreen>
#include <QKeyEvent>

// NspCompletionFilter::eventFilter
// Intercepts Tab, Enter, Return, and Escape key presses when the completion
// popup is visible. Tab/Enter/Return trigger insertion of the selected
// completion and hide the popup. Escape just hides the popup. All other keys
// (including arrow keys for popup navigation) are passed to the base QCompleter.
bool NspCompletionFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        int key = ke->key();
        if (key == Qt::Key_Tab || key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Escape) {
            if (m_nspCompleter && m_nspCompleter->isPopupVisible()) {
                if (key == Qt::Key_Tab || key == Qt::Key_Enter || key == Qt::Key_Return) {
                    // Use the stored selected name, or fall back to currentCompletion
                    QString name = m_nspCompleter->selectedName();
                    if (name.isEmpty())
                        name = currentCompletion();
                    if (!name.isEmpty())
                        m_nspCompleter->insertCompletion(name);
                    m_nspCompleter->popup()->hide();
                    m_nspCompleter->popup()->infoLabel()->hide();
                } else {
                    // Escape key: just dismiss the popup
                    m_nspCompleter->popup()->hide();
                    m_nspCompleter->popup()->infoLabel()->hide();
                }
                return true; // Consume the event so the editor doesn't process it
            }
        }
    }
    return QCompleter::eventFilter(obj, event);
}

// CompletionPopup constructor
// Creates the popup tree view and an associated info label (positioned
// beside the popup as a tooltip-style window) that shows details about
// the currently highlighted completion item.
CompletionPopup::CompletionPopup(QWidget *parent)
    : QTreeView(parent)
    , m_infoLabel(new QLabel)
{
    m_infoLabel->setWindowFlags(Qt::ToolTip);
    m_infoLabel->setTextFormat(Qt::PlainText);
    m_infoLabel->setWordWrap(true);
    QFont f = m_infoLabel->font();
    f.setFamily("Consolas");
    f.setPointSize(9);
    m_infoLabel->setFont(f);
    m_infoLabel->setFixedWidth(300);
}

// Hides the info label whenever the popup is hidden.
void CompletionPopup::hideEvent(QHideEvent *event)
{
    m_infoLabel->hide();
    QTreeView::hideEvent(event);
}

// NspCompleter constructor
// Sets up the completion model (2 columns: name+signature, description),
// configures the NspCompletionFilter with popup completion mode, case-
// insensitive matching, and a max of 8 visible items. Connects the
// activated signal to insertCompletion and selection changes to
// onCurrentIndexChanged for info label updates.
NspCompleter::NspCompleter(CodeEditor *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_model(new QStandardItemModel(this))
    , m_popup(new CompletionPopup)
    , m_completionStartPos(0)
{
    m_model->setColumnCount(2);

    m_completer = new NspCompletionFilter(m_model, this);
    m_completer->setNspCompleter(this);
    m_completer->setWidget(m_editor);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setMaxVisibleItems(8);
    m_completer->setFilterMode(Qt::MatchContains);
    // Filter on Qt::EditRole which stores the raw name (without params/signature)
    m_completer->setCompletionRole(Qt::EditRole);

    // Configure the popup tree view: no header, flat layout, no indentation
    m_popup->setHeaderHidden(true);
    m_popup->setRootIsDecorated(false);
    m_popup->setIndentation(0);
    m_popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Column 0 (name) sizes to content; column 1 (description) stretches
    m_popup->header()->setStretchLastSection(false);
    m_popup->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_popup->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_popup->setUniformRowHeights(true);

    m_completer->setPopup(m_popup);

    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &NspCompleter::insertCompletion);
    connect(m_popup->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NspCompleter::onCurrentIndexChanged);
}

// NspCompleter::onCurrentIndexChanged
// Called when the user navigates the completion popup. Stores the selected
// item's name (from Qt::UserRole) into m_selectedName and looks up the
// full NspNameSpace entry to display type, description, params, and returns
// in a tooltip-style info label positioned beside the popup.
void NspCompleter::onCurrentIndexChanged(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_selectedName.clear();
        m_popup->infoLabel()->hide();
        return;
    }
    // Retrieve the raw name stored in UserRole data
    QString name = index.sibling(index.row(), 0).data(Qt::UserRole).toString();
    m_selectedName = name;
    // Build the full qualified key for lookup (e.g., "math.sin" or "print")
    QString fullKey = m_currentNamespace.isEmpty() ? name : m_currentNamespace + "." + name;
    const NspNameSpace::Entry *entry = NspNameSpace::instance().findNode(fullKey);
    if (!entry) {
        m_popup->infoLabel()->hide();
        return;
    }

    // Compose the info label text from available entry fields
    QStringList lines;
    if (!entry->type.isEmpty()) lines << QString("[%1]").arg(entry->type);
    if (!entry->desc.isEmpty()) lines << entry->desc;
    if (!entry->params.isEmpty()) lines << "Params: " + entry->params;
    if (!entry->returns.isEmpty()) lines << "Returns: " + entry->returns;

    if (lines.isEmpty()) {
        m_popup->infoLabel()->hide();
        return;
    }

    m_popup->infoLabel()->setText(lines.join("\n"));
    m_popup->infoLabel()->adjustSize();

    // Position the info label to the right of the popup, or to the left
    // if it would overflow off the right edge of the screen
    QRect popupGeom = m_popup->geometry();
    QPoint infoPos = popupGeom.topRight();
    if (infoPos.x() + 310 > QGuiApplication::primaryScreen()->availableGeometry().right())
        infoPos.setX(popupGeom.topLeft().x() - 310);
    m_popup->infoLabel()->move(infoPos);
    m_popup->infoLabel()->show();
}

// Returns true if the completion popup is currently visible to the user.
bool NspCompleter::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

// NspCompleter::currentWordPrefix
// Extracts the identifier prefix (letters, digits, underscore, dollar sign)
// immediately before the cursor position. Used to determine what text the
// completion should replace.
QString NspCompleter::currentWordPrefix() const
{
    QTextCursor cursor = m_editor->textCursor();
    int pos = cursor.position();
    QTextBlock block = cursor.block();
    QString text = block.text();
    int col = cursor.positionInBlock();

    // Walk backwards to find the start of the current identifier
    int start = col;
    while (start > 0 && (text[start - 1].isLetterOrNumber() || text[start - 1] == '_' || text[start - 1] == '$'))
        --start;

    return text.mid(start, col - start);
}

// NspCompleter::triggerCompletion
// Called when the user has typed 2+ identifier characters (without a preceding
// dot). Determines the current namespace context from getLabel(), looks up
// matching entries, populates the model, calculates the completion start
// position, and shows the popup.
void NspCompleter::triggerCompletion(int cursorPos)
{
    // Ensure the namespace data is loaded from embedded resources
    NspNameSpace::instance().loadResources();

    QTextCursor cursor = m_editor->textCursor();
    QString label = m_editor->getLabel(cursorPos, false);

    // If the label contains a dot, split into namespace and member prefix
    if (label.contains('.')) {
        int dotPos = label.lastIndexOf('.');
        m_currentNamespace = label.left(dotPos);
        label = label.mid(dotPos + 1);
    } else {
        m_currentNamespace.clear();
    }

    QString prefix = label.toLower();

    // Fetch entries from the global scope or the specified namespace
    QList<NspNameSpace::Entry> entries;
    if (!m_currentNamespace.isEmpty()) {
        entries = NspNameSpace::instance().getList(m_currentNamespace);
    } else {
        entries = NspNameSpace::instance().getList(QString());
    }

    // Sort alphabetically by name for consistent display
    std::sort(entries.begin(), entries.end(),
        [](const NspNameSpace::Entry &a, const NspNameSpace::Entry &b) { return a.name < b.name; });

    populateModel(entries, prefix);
    if (m_model->rowCount() == 0) return;

    // Record where the typed prefix starts so insertCompletion can replace it
    m_completionStartPos = cursorPos - prefix.length();

    m_completer->setCompletionPrefix(prefix);

    QRect rect = m_editor->cursorRect();
    rect.setWidth(350);
    m_completer->complete(rect);
}

// NspCompleter::popupForDot
// Called when the user types a dot (.) after an identifier. Shows completion
// for all members of the namespace preceding the dot. Also adds universal
// methods (gettype, length, tostring) if not already present.
void NspCompleter::popupForDot()
{
    NspNameSpace::instance().loadResources();

    QTextCursor cursor = m_editor->textCursor();
    QString label = m_editor->getLabel(cursor.position(), false);
    m_currentNamespace = label;

    QList<NspNameSpace::Entry> entries = NspNameSpace::instance().getList(label);

    // Add universal methods that exist on all objects
    static const QStringList universal = {"gettype", "length", "tostring"};
    for (const QString &name : universal) {
        bool found = false;
        for (const NspNameSpace::Entry &e : entries) {
            if (e.name == name) { found = true; break; }
        }
        if (!found) {
            NspNameSpace::Entry e;
            e.name = name;
            e.fullName = label.isEmpty() ? name : label + "." + name;
            e.type = "function";
            e.desc = "";
            e.params = "";
            e.returns = "";
            entries.append(e);
        }
    }
    std::sort(entries.begin(), entries.end(),
        [](const NspNameSpace::Entry &a, const NspNameSpace::Entry &b) { return a.name < b.name; });

    // No prefix to filter — show all members of the namespace
    populateModel(entries, QString());
    if (m_model->rowCount() == 0) return;

    // Completion starts at the current cursor position (after the dot)
    m_completionStartPos = cursor.position();

    m_completer->setCompletionPrefix("");

    QRect rect = m_editor->cursorRect();
    rect.setWidth(350);
    m_completer->complete(rect);
}

// NspCompleter::populateModel
// Fills the QStandardItemModel with rows for each matching entry. Each row
// has two columns: (1) icon + display name with optional params, and
// (2) description snippet. Icons are type-colored Unicode hexagons:
// orange=functions, blue=tables, green=strings, purple=booleans, grey=null/other.
// The raw name is stored in Qt::EditRole (for filtering) and Qt::UserRole
// (for retrieval on selection).
void NspCompleter::populateModel(const QList<NspNameSpace::Entry> &entries, const QString &prefix)
{
    m_model->clear();
    m_model->setColumnCount(2);

    for (const NspNameSpace::Entry &e : entries) {
        // Select icon shape and color based on the entry's type
        QString icon;
        QColor iconColor;
        if (e.type == "function")    { icon = u8"\u2B22"; iconColor = QColor(220, 147, 40); }
        else if (e.type == "table")  { icon = u8"\u2B22"; iconColor = QColor(76, 153, 215); }
        else if (e.type == "string")  { icon = u8"\u2B22"; iconColor = QColor(30, 150, 70); }
        else if (e.type == "boolean") { icon = u8"\u2B22"; iconColor = QColor(160, 90, 200); }
        else if (e.type == "null")   { icon = u8"\u2B22"; iconColor = QColor(140, 140, 140); }
        else                         { icon = u8"\u2B22"; iconColor = QColor(180, 180, 180); }

        // Show params after function name, e.g. "print(x, ...)"
        QString displayText;
        if (e.type == "function" && !e.params.isEmpty())
            displayText = e.name + e.params;
        else
            displayText = e.name;

        QList<QStandardItem *> row;

        QStandardItem *nameItem = new QStandardItem(icon + " " + displayText);
        nameItem->setForeground(iconColor);
        QFont font = nameItem->font();
        font.setFamily("Consolas");
        font.setPointSize(9);
        nameItem->setFont(font);
        nameItem->setData(e.name, Qt::EditRole);    // Filter role: raw name for matching
        nameItem->setData(e.name, Qt::UserRole);     // Selection role: name for insertion
        nameItem->setData(e.type, Qt::UserRole + 1); // Type for post-insert behavior
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        row << nameItem;

        // Build a short description for the second column
        QString tip;
        if (!e.desc.isEmpty()) {
            tip = e.desc;
            if (e.type == "table") tip.prepend("(table) ");
        } else if (e.type == "table") {
            tip = "(table)";
        } else if (e.type == "boolean") {
            tip = "(boolean)";
        }

        QStandardItem *descItem = new QStandardItem(tip);
        QFont dfont = descItem->font();
        dfont.setFamily("Consolas");
        dfont.setPointSize(9);
        descItem->setFont(dfont);
        descItem->setForeground(QColor(120, 120, 120));
        row << descItem;

        m_model->appendRow(row);
    }
}

// NspCompleter::insertCompletion
// Replaces the typed prefix with the completion text. For function entries,
// appends "()" and positions the cursor inside the parentheses. For table
// entries (handled by the caller in the main editor), "." would be appended.
void NspCompleter::insertCompletion(const QString &completion)
{
    m_popup->infoLabel()->hide();

    // Look up the entry to determine whether to append function parentheses
    const NspNameSpace::Entry *entry =
        NspNameSpace::instance().findNode(m_currentNamespace.isEmpty()
            ? completion : m_currentNamespace + "." + completion);

    QTextCursor cursor = m_editor->textCursor();

    // Select and remove the typed prefix before inserting the completion
    int prefixLen = cursor.position() - m_completionStartPos;
    if (prefixLen > 0) {
        cursor.setPosition(m_completionStartPos, QTextCursor::MoveAnchor);
        cursor.setPosition(m_completionStartPos + prefixLen, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    // For functions, append "()" so the user can fill in arguments
    QString insertion = completion;
    bool isFunction = entry && entry->type == "function";
    if (isFunction)
        insertion += "()";

    cursor.insertText(insertion);

    // Position cursor inside the parentheses for functions
    if (isFunction)
        cursor.setPosition(cursor.position() - 1);

    m_editor->setTextCursor(cursor);
}