// MemTreeView.cpp - Implementation of the NSP memory viewer widget.
// Builds a tree model from the NSP interpreter state showing Globals,
// Locals, and This sections. Table members are expanded first, followed
// by non-table members. Clicking shows details in a panel below.

#include "MemTreeView.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>

// MemTreeView constructor
// Creates the tree view (3 columns: Name, Type, Value) and a read-only
// detail panel below it, arranged in a vertical splitter. Clicking a
// tree item updates the detail panel with its type and value.
MemTreeView::MemTreeView(QWidget *parent)
    : QWidget(parent)
    , m_nsp(nullptr)
{
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Value");

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setIndentation(8);  // Compact indentation for nested tables

    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setMaximumHeight(100);  // Limited height for the detail area

    // Vertical splitter: tree on top, detail panel below
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_treeView);
    splitter->addWidget(m_detailView);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    // When a tree item is clicked, show its type and value in the detail panel
    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        QModelIndex nameIdx = m_model->index(index.row(), 0, index.parent());
        QModelIndex typeIdx = m_model->index(index.row(), 1, index.parent());
        QModelIndex valueIdx = m_model->index(index.row(), 2, index.parent());
        QString name = m_model->data(nameIdx).toString();
        QString type = m_model->data(typeIdx).toString();
        QString value = m_model->data(valueIdx).toString();
        m_detailView->setText(QString("Type: %1\nValue: %2").arg(type, value));
    });
}

// Sets the NSP state pointer and rebuilds the entire tree from it.
void MemTreeView::setNspState(nsp_state *N)
{
    m_nsp = N;
    buildTree();
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

// MemTreeView::addChildren
// Recursively populates tree items for children of a table object.
// Table-type children are added first (and expanded recursively), then
// non-table children are listed after. Depth is limited to 10 levels.
void MemTreeView::addChildren(QStandardItem *parentItem, obj_t *tableObj, int depth)
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
        row << new QStandardItem(type);
        row << new QStandardItem(value);
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
        row << new QStandardItem(type);
        row << new QStandardItem(value);
        parentItem->appendRow(row);
    }
}

// MemTreeView::buildTree
// Clears and rebuilds the tree model with three top-level sections:
// Globals (m_nsp->g), Locals (m_nsp->context->l), and This (m_nsp->context->t).
// Only populates Locals and This if there is an active context (breakpoint).
void MemTreeView::buildTree()
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

    // Expand the Globals section and auto-size columns
    m_treeView->expand(globalsItem->index());
    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);
    // Give the Name column extra width for readability
    int nameWidth = m_treeView->columnWidth(0);
    m_treeView->setColumnWidth(0, nameWidth * 2);
}