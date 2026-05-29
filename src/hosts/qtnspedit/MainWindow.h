#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>

// Forward declarations — avoids pulling in full headers for types only used as pointers
class QTabWidget;
class QSplitter;
class CodeEditor;
class OutputPanel;
class ScriptRunner;
class FindBar;
class NspCompleter;
class FileBrowser;
class DebugPanel;
class QLabel;

// MainWindow is the main application window for qtnspedit (NSP script editor).
// It manages a tabbed interface where each tab contains a vertical QSplitter
// with a CodeEditor on top and an OutputPanel on the bottom. A horizontal
// splitter holds the file browser (left) and editor tabs (center), and the
// memory viewer panel can slide in to the right of the editor area.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Returns the CodeEditor widget in the currently active tab, or nullptr
    CodeEditor *activeCodeEditor() const;
    // Returns the OutputPanel widget in the currently active tab, or nullptr
    OutputPanel *activeOutputPanel() const;
    // Returns the file path stored on the current tab's property, or empty string
    QString activeFileName() const;

    // Opens a file by path into a new tab (used by File>Open and FileBrowser double-click)
    void loadFile(const QString &fileName);
    // Appends text to the current tab's output panel
    void appendOutput(const QString &text);
    // Clears the current tab's output panel
    void clearOutput();
    // Sets the text shown in the left portion of the status bar
    void setStatus(const QString &msg);

protected:
    // Saves window geometry and output panel height; prompts for unsaved changes before closing
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu slots
    void newFile();                  // Creates a new untitled .ns tab
    void openFile();                 // Shows file dialog to open an .ns/.nsp file
    void openFolder();               // Shows folder dialog to set the file browser root
    void saveFile();                 // Saves the active editor content (prompts for path if untitled)
    void saveFileAsHTML();           // Exports the active editor content as syntax-highlighted HTML
    void closeTab(int index);        // Closes and deletes the tab at the given index; creates a new tab if none remain

    // Script menu slots
    void runScript();                // Runs the active script; resumes if paused at breakpoint
    void stopScript();               // Stops a running script
    void viewDebugPanel();           // Toggles the debug panel visibility

    // Edit menu slots
    void showFindBar();              // Shows the inline find bar on the active editor
    void findNext();                 // Advances to the next search match
    void findPrev();                 // Advances to the previous search match
    void formatCode();               // Re-indents the active .ns/.nsp file using NspFormatter
    void toggleComment();            // Toggles // comment prefix on selected lines

    // Help menu slots
    void helpOnline();               // Opens the NSP website in the default browser
    void helpSyntax();               // Opens the NSP syntax reference in the default browser
    void about();                    // Shows the About dialog

    // ScriptRunner signal handlers
    void onOutputReady(const QString &text);         // Appends script stdout/stderr to the output panel
    void onScriptFinished(bool error, const QString &errbuf); // Updates status bar when script ends; shows errors if any
    void onBreakpointHit();                          // Opens debug panel and shows breakpoint status when debug.break() is hit

private:
    // UI setup helpers called from the constructor
    void createMenus();              // Builds the File/Edit/Script/Help menu bar
    void createToolBar();             // Creates the icon-only toolbar (New, Open, Save, Run)
    void createStatusBar();          // Creates the status bar with message label and line number label

    // Creates a new editor+output tab widget. isNsp=true enables syntax highlighting and autocomplete for .ns/.nsp files
    QWidget *createTab(bool isNsp = true);
    // Called when the active tab changes — updates window title and re-activates the completer/highlighter
    void switchTab(int index);
    // Configures the syntax highlighter's mode (.ns pure script vs .nsp template) based on file extension
    void configureHighlighter(CodeEditor *editor, const QString &fileName);
    // Finds the FindBar child widget in the active editor's hierarchy
    FindBar *activeFindBar() const;
    // Opens a file by path from the file browser (delegates to loadFile)
    void openFileFromBrowser(const QString &filePath);
    // Updates the tab's label text: prepends * if document is modified, shows filename or "Untitled"
    void updateTabText(QWidget *tab);

    // Central tab widget holding one QSplitter per open file
    QTabWidget *m_tabWidget;
    // Horizontal splitter hosting file browser (left) and tab widget (center)
    QSplitter *m_mainSplitter;
    // Left-side file tree browser (QTreeView + QFileSystemModel)
    FileBrowser *m_fileBrowser;
    // Right-side debug panel (hidden until F6 or breakpoint)
    DebugPanel *m_debugPanel;
    // Background thread that executes NSP scripts
    ScriptRunner *m_scriptRunner;
    // Most-recently-activated NSP autocompleter (switched in switchTab)
    NspCompleter *m_completer;
    // Left status bar label showing status messages
    QLabel *m_statusLabel;
    // Right status bar label showing the current line number
    QLabel *m_lineLabel;
    // Saved output panel height for persistence across sessions
    int m_outputHeight;
    // Script menu Run action (text changes between "Run" and "Resume")
    QAction *m_runAction;
    // Script menu and toolbar Stop actions (enabled only when a script is running)
    QAction *m_stopAction;
    QAction *m_toolbarStopAction;
};

#endif // MAINWINDOW_H