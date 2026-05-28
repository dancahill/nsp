// FileBrowser.cpp - Implementation of the sidebar file browser widget.
// Shows a compact directory tree with hidden column headers and no root
// decoration. Directories expand/collapse on activation; files emit
// fileActivated for opening in the editor.

#include "FileBrowser.h"

#include <QMouseEvent>
#include <QHeaderView>

// FileBrowser constructor
// Sets up the filesystem model (directories and files, no dot/dotdot),
// configures the tree view for compact display, and connects activation
// and double-click handlers.
FileBrowser::FileBrowser(QWidget *parent)
    : QTreeView(parent)
    , m_model(new QFileSystemModel(this))
{
    // Show all entries except . and ..
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    m_model->setNameFilterDisables(false);
    setModel(m_model);

    // Compact listing: no tree lines, small indentation, left margin
    setRootIsDecorated(false);
    setIndentation(16);
    setViewportMargins(4, 0, 0, 0);
    setItemsExpandable(true);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    // Hide the header and all columns except the name column
    header()->hide();
    for (int i = 1; i < m_model->columnCount(); i++)
        hideColumn(i);

    setMinimumWidth(150);
    setMaximumWidth(400);

    connect(this, &QTreeView::activated, this, &FileBrowser::onActivated);
}

// Sets the root path of the filesystem model and updates the tree's root index.
void FileBrowser::setRootPath(const QString &path)
{
    QModelIndex root = m_model->setRootPath(path);
    setRootIndex(root);
}

// Returns the filesystem model's current root path.
QString FileBrowser::rootPath() const
{
    return m_model->rootPath();
}

// FileBrowser::onActivated
// Handles the activated signal (Enter key or single-click depending on
// system settings). For directories, toggles expand/collapse. For files,
// emits fileActivated so MainWindow can open the file.
void FileBrowser::onActivated(const QModelIndex &index)
{
    QString path = m_model->filePath(index);
    if (m_model->isDir(index)) {
        if (isExpanded(index))
            collapse(index);
        else
            expand(index);
        return;
    }
    emit fileActivated(path);
}

// FileBrowser::mouseDoubleClickEvent
// Handles double-click separately from activation. For directories,
// toggles expand/collapse. For files, emits fileActivated.
void FileBrowser::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        QTreeView::mouseDoubleClickEvent(event);
        return;
    }
    if (m_model->isDir(index)) {
        if (isExpanded(index))
            collapse(index);
        else
            expand(index);
        return;
    }
    QString path = m_model->filePath(index);
    emit fileActivated(path);
}