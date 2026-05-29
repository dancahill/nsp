// DebugPanel.cpp - Implementation of the NSP debug panel widget.
// Builds a tree model from the NSP interpreter state showing Globals,
// Locals, and This sections. Table members are expanded first, then
// non-table members. Clicking shows details in a panel below.
// Includes Run/Stop buttons connected to script execution.

#include "DebugPanel.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>

// Green play-triangle icon for the Run button (matches toolbar icon).
static QPixmap GetRunIcon()
{
    static const char *run_xpm[] = {
        "22 22 3 1", "  c None", ". c #00AA00", "x c #00CC00",
        "                      ", "                      ",
        "                      ", "                      ",
        "  ....                ", "  ....xx              ",
        "  ....xxxx            ", "  ....xxxxxx          ",
        "  ....xxxxxxxx        ", "  ....xxxxxxxxxx      ",
        "  ....xxxxxxxxxxxx    ", "  ....xxxxxxxxxxxxxx  ",
        "  ....xxxxxxxxxxxxxx  ", "  ....xxxxxxxxxxxx    ",
        "  ....xxxxxxxxxx      ", "  ....xxxxxxxx        ",
        "  ....xxxxxx          ", "  ....xxxx            ",
        "  ....xx              ", "  ....                ",
        "                      ", "                      "
    };
    return QPixmap(run_xpm);
}

// Red square icon for the Stop button. Grey when disabled.
static QPixmap GetStopIcon()
{
    static const char *stop_xpm[] = {
        "22 22 2 1", "  c None", ". c #CC0000",
        "                      ", "                      ",
        "                      ", "                      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "      ..........      ", "      ..........      ",
        "                      ", "                      "
    };
    return QPixmap(stop_xpm);
}

// DebugPanel constructor
// Creates the tree view (3 columns: Name, Value, Type), a detail panel below it,
// and Run/Stop icon buttons at the top, arranged in a vertical layout.
DebugPanel::DebugPanel(QWidget *parent)
    : QWidget(parent)
    , m_nsp(nullptr)
{
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(QStringList() << "Name" << "Value" << "Type");

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setIndentation(8);

    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setMaximumHeight(100);

    // Run (green play) and Stop (red square) icon-only buttons
    m_runButton = new QPushButton(QIcon(GetRunIcon()), QString(), this);
    m_runButton->setToolTip("Run (F5)");
    m_runButton->setFixedSize(26, 26);

    m_stopButton = new QPushButton(QIcon(GetStopIcon()), QString(), this);
    m_stopButton->setToolTip("Stop script");
    m_stopButton->setFixedSize(26, 26);
    m_stopButton->setEnabled(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_runButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    // Hint label at the bottom
    QLabel *hintLabel = new QLabel("Use lib.debug.break() to set a breakpoint", this);
    hintLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");

    // Vertical splitter: tree on top, detail panel below
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_treeView);
    splitter->addWidget(m_detailView);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(buttonLayout);
    layout->addWidget(splitter);
    layout->addWidget(hintLabel);

    // When a tree item is selected (click or keyboard), show its type and value
    auto updateDetail = [this](const QModelIndex &index) {
        QModelIndex nameIdx = m_model->index(index.row(), 0, index.parent());
        QModelIndex valueIdx = m_model->index(index.row(), 1, index.parent());
        QModelIndex typeIdx = m_model->index(index.row(), 2, index.parent());
        QString name = m_model->data(nameIdx).toString();
        QString value = m_model->data(valueIdx).toString();
        QString type = m_model->data(typeIdx).toString();
        m_detailView->setText(QString("Type: %1\nValue: %2").arg(type, value));
    };

    // Forward button clicks to signals
    connect(m_runButton, &QPushButton::clicked, this, &DebugPanel::runClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &DebugPanel::stopClicked);

    // Click updates the detail panel
    connect(m_treeView, &QTreeView::clicked, this, updateDetail);
    // Keyboard navigation updates the detail panel via selection model
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
        QModelIndex nameIdx = m_model->index(current.row(), 0, current.parent());
        QModelIndex valueIdx = m_model->index(current.row(), 1, current.parent());
        QModelIndex typeIdx = m_model->index(current.row(), 2, current.parent());
        QString name = m_model->data(nameIdx).toString();
        QString value = m_model->data(valueIdx).toString();
        QString type = m_model->data(typeIdx).toString();
        m_detailView->setText(QString("Type: %1\nValue: %2").arg(type, value));
    });

}

// Sets the NSP state pointer and rebuilds the entire tree from it.
void DebugPanel::setNspState(nsp_state *N)
{
    m_nsp = N;
    buildTree();
}

// Updates button state: Run shows "Resume" tooltip when running, Stop enabled.
void DebugPanel::setRunning(bool running)
{
    m_runButton->setToolTip(running ? "Resume (F5)" : "Run (F5)");
    m_stopButton->setEnabled(running);
}

// objectType: Converts an NSP type code to a human-readable string.
static QString objectType(unsigned short type)
{
    switch (type) {
    case NT_NULL:    return "null";
    case NT_BOOLEAN: return "boolean";
    case NT_NUMBER:  return "number";
    case NT_STRING:  return "string";
    case NT_NFUNC:   return "nfunc";
    case NT_CFUNC:   return "cfunc";
    case NT_TABLE:   return "table";
    case NT_CDATA:   return "cdata";
    default:         return "unknown";
    }
}

// objectValue: Returns a string representation of an NSP object's value.
// Tables show "[table]", functions show "name()", strings show their content,
// numbers show their numeric value, etc.
static QString objectValue(obj_t *obj)
{
    if (!obj || !obj->val) return "null";
    switch (obj->val->type) {
    case NT_NULL:    return "null";
    case NT_BOOLEAN: return obj->val->d.num ? "true" : "false";
    case NT_NUMBER:  return QString::number(obj->val->d.num);
    case NT_STRING:  return QString::fromUtf8(obj->val->d.str, obj->val->size);
    case NT_NFUNC:
    case NT_CFUNC:   return QString("%1()").arg(obj->name);
    case NT_TABLE:   return "[table]";
    case NT_CDATA:   return QString::fromUtf8(obj->val->d.cdata->obj_type);
    default:         return "[unknown]";
    }
}

// DebugPanel::addChildren
// Recursively populates tree items for children of a table object.
// Table-type children are added first (and expanded recursively), then
// non-table children are listed after. Depth is limited to 10 levels.
void DebugPanel::addChildren(QStandardItem *parentItem, obj_t *tableObj, int depth)
{
    if (depth > 10 || !tableObj || !tableObj->val || tableObj->val->type != NT_TABLE)
        return;

    // First pass: collect table-type children (these get expandable sub-nodes)
    QList<obj_t*> tableChildren;
    for (obj_t *child = tableObj->val->d.table.f; child; child = child->next) {
        if (child->val && child->val->type == NT_TABLE)
            tableChildren.append(child);
    }
    // Add table children first, with recursive expansion
    for (obj_t *child : tableChildren) {
        QString name = child->name;
        QString type = objectType(child->val->type);
        QString value = objectValue(child);

        QList<QStandardItem*> row;
        row << new QStandardItem(name);
        row << new QStandardItem(value);
        row << new QStandardItem(type);
        parentItem->appendRow(row);
        addChildren(row[0], child, depth + 1);
    }

    // Second pass: collect non-table children (scalars, functions, etc.)
    QList<obj_t*> nonTableChildren;
    for (obj_t *child = tableObj->val->d.table.f; child; child = child->next) {
        if (child->val && child->val->type != NT_TABLE)
            nonTableChildren.append(child);
    }
    // Add non-table children after tables so they appear below in the list
    for (obj_t *child : nonTableChildren) {
        QString name = child->name;
        // Append "()" to function names for visual clarity
        if (child->val && (child->val->type == NT_NFUNC || child->val->type == NT_CFUNC))
            name += "()";
        QString type = objectType(child->val ? child->val->type : NT_NULL);
        QString value = objectValue(child);

        QList<QStandardItem*> row;
        row << new QStandardItem(name);
        row << new QStandardItem(value);
        row << new QStandardItem(type);
        parentItem->appendRow(row);
    }
}

// DebugPanel::buildTree
// Clears and rebuilds the tree model with three top-level sections:
// Globals (m_nsp->g), Locals (m_nsp->context->l), and This (m_nsp->context->t).
// Only populates Locals and This if there is an active context (breakpoint).
void DebugPanel::buildTree()
{
    m_model->removeRows(0, m_model->rowCount());

    if (!m_nsp) return;

    // Three top-level sections for the different NSP scopes
    QStandardItem *globalsItem = new QStandardItem("Globals");
    QStandardItem *localsItem = new QStandardItem("Locals");
    QStandardItem *thisItem = new QStandardItem("This");
    m_model->appendRow(globalsItem);
    m_model->appendRow(localsItem);
    m_model->appendRow(thisItem);

    // Always populate globals from the main NSP state table
    addChildren(globalsItem, &m_nsp->g, 0);

    // Locals and This only available when stopped at a breakpoint
    if (m_nsp->context) {
        addChildren(localsItem, &m_nsp->context->l, 0);
        addChildren(thisItem, &m_nsp->context->t, 0);
    }

    // Expand the Globals section and size columns:
    // Type column fixed at ~8 chars, Name and Value share the rest evenly
    m_treeView->expand(globalsItem->index());
    int typeWidth = m_treeView->fontMetrics().horizontalAdvance("xxxxxxxx") + 16;
    m_treeView->setColumnWidth(2, typeWidth);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
}