// DebugPanel.h - A QWidget that displays NSP runtime memory in a tree view
// alongside a Run/Resume button for script control. Shows Globals, Locals,
// and This sections with expandable table members. Auto-opens when
// debug.break() pauses script execution. Clicking an item shows its Type and
// Value in a detail panel below the tree.

#ifndef DEBUGPANEL_H
#define DEBUGPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QTextEdit>
#include <QStandardItemModel>
#include <QPushButton>

extern "C" {
#include "nsp/nsp.h"
}

class DebugPanel : public QWidget {
    Q_OBJECT
public:
    explicit DebugPanel(QWidget *parent = nullptr);

    // Sets the NSP state pointer and rebuilds the tree from its memory.
    void setNspState(nsp_state *N);

    // Updates button text based on whether a script is currently running.
    void setRunning(bool running);

signals:
    void runClicked();
    void stopClicked();

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
    QPushButton *m_runButton;      // Run/Resume script button
    QPushButton *m_stopButton;    // Stop script button
    QStandardItemModel *m_model;  // The model backing the tree view
    nsp_state *m_nsp;            // The NSP interpreter state to inspect
};

#endif // DEBUGPANEL_H