// FileBrowser.h - A QTreeView-based file browser widget for the left sidebar.
// Uses QFileSystemModel to display directory contents. Emits fileActivated
// when a file is double-clicked so MainWindow can open it. Directories are
// expanded/collapsed on click rather than activated.

#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QTreeView>
#include <QFileSystemModel>

class FileBrowser : public QTreeView {
    Q_OBJECT
public:
    explicit FileBrowser(QWidget *parent = nullptr);

    // Sets the root directory path for the file browser.
    void setRootPath(const QString &path);
    // Returns the current root directory path.
    QString rootPath() const;

signals:
    // Emitted when a file is double-clicked or activated (not directories).
    void fileActivated(const QString &filePath);

protected:
    // Handles double-click: expands/collapses directories, emits fileActivated for files.
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    // Handles activation (Enter key or single click, depending on system):
    // expands/collapses directories, emits fileActivated for files.
    void onActivated(const QModelIndex &index);

private:
    QFileSystemModel *m_model;  // The filesystem model backing this tree view
};

#endif // FILEBROWSER_H