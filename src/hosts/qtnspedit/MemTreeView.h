// MemTreeView.h - A QWidget (not QDialog) that displays NSP runtime memory
// in a tree view. Shows Globals, Locals, and This sections with expandable
// table members. Auto-opens when debug.break() pauses script execution.
// Clicking an item shows its Type and Value in a detail panel below the tree.

#ifndef MEMTREEVIEW_H
#define MEMTREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QTextEdit>
#include <QStandardItemModel>

extern "C" {
#include "nsp/nsp.h"
}

class MemTreeView : public QWidget {
    Q_OBJECT
public:
    explicit MemTreeView(QWidget *parent = nullptr);

    // Sets the NSP state pointer and rebuilds the tree from its memory.
    void setNspState(nsp_state *N);

private:
    // Clears and rebuilds the tree model from the current NSP state,
    // populating Globals, Locals, and This sections.
    void buildTree();
    // Recursively adds children of a table object to the tree model.
    // Tables are listed first (expanded), then non-table members.
    // Depth-limited to 10 levels to avoid infinite recursion.
    void addChildren(QStandardItem *parentItem, obj_t *tableObj, int depth);

    QTreeView *m_treeView;        // The tree widget showing name/type/value columns
    QTextEdit *m_detailView;      // Shows type and value of the clicked item
    QStandardItemModel *m_model;  // The model backing the tree view
    nsp_state *m_nsp;            // The NSP interpreter state to inspect
};

#endif // MEMTREEVIEW_H