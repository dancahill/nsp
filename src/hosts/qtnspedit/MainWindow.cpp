// MainWindow.cpp — Implementation of the main application window for qtnspedit.
// Manages the tabbed editor interface, menus, toolbar, script execution, and file I/O.

#include "MainWindow.h"
#include "CodeEditor.h"
#include "NspSyntaxHighlighter.h"
#include "NspCompleter.h"
#include "NspNameSpace.h"
#include "OutputPanel.h"
#include "ScriptRunner.h"
#include "FindBar.h"
#include "DebugPanel.h"
#include "Settings.h"
#include "NspFormatter.h"
#include "FileBrowser.h"

#include <QTabWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFont>
#include <QIcon>
#include <QSize>
#include <QPixmap>

// ---------------------------------------------------------------------------
// Constructor — sets up the entire main window: file browser, tab widget,
// memory viewer, menus, toolbar, status bar, signal connections, and opens
// an initial untitled tab.
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scriptRunner(nullptr)
    , m_completer(nullptr)
    , m_outputHeight(75)
{
    // Load the built-in NSP name database (from the compiled XML resource)
    NspNameSpace::instance().loadResources();

    setWindowTitle("NSP Editor");
    setWindowIcon(QIcon(":/resources/app.ico"));
    resize(800, 600);

    // Restore saved window position and size
    QRect geometry = Settings::loadWindowGeometry();
    setGeometry(geometry);

    // Central tab widget — each tab holds a QSplitter(CodeEditor, OutputPanel)
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    // Left-side file browser — hidden by default unless a last-used directory exists
    m_fileBrowser = new FileBrowser(this);
    QString lastDir = Settings::loadLastDir();
    if (!lastDir.isEmpty() && QDir(lastDir).exists()) {
        m_fileBrowser->setRootPath(lastDir);
        m_fileBrowser->setVisible(true);
    } else {
        m_fileBrowser->setVisible(false);
    }

    // Horizontal splitter: file browser (left) | tab widget (right)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_fileBrowser);
    m_mainSplitter->addWidget(m_tabWidget);
    m_mainSplitter->setStretchFactor(0, 0);  // file browser keeps its size
    m_mainSplitter->setStretchFactor(1, 1);  // tabs stretch to fill
    m_mainSplitter->setSizes(QList<int>() << 200 << 600);

    // Debug panel — initially hidden; slides in to the right of the editor area
    m_debugPanel = new DebugPanel(this);
    m_debugPanel->setVisible(false);

    // Outer horizontal splitter: editor area (left) | debug panel (right)
    QSplitter *horzSplitter = new QSplitter(Qt::Horizontal, this);
    horzSplitter->addWidget(m_mainSplitter);
    horzSplitter->addWidget(m_debugPanel);
    horzSplitter->setStretchFactor(0, 1);
    horzSplitter->setStretchFactor(1, 0);
    setCentralWidget(horzSplitter);

    // Build menus, toolbar, and status bar
    createMenus();
    createToolBar();
    createStatusBar();

    // Connect tab widget signals
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::switchTab);

    // Connect file browser double-click
    connect(m_fileBrowser, &FileBrowser::fileActivated, this, &MainWindow::openFileFromBrowser);

    // Connect debug panel Run/Stop buttons
    connect(m_debugPanel, &DebugPanel::runClicked, this, &MainWindow::runScript);
    connect(m_debugPanel, &DebugPanel::stopClicked, this, &MainWindow::stopScript);

    // Restore saved output panel height
    m_outputHeight = Settings::loadOutputHeight();

    // Start with one untitled tab
    newFile();
}

// ---------------------------------------------------------------------------
// Destructor — resumes and waits for any running script thread before destruction
// ---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    if (m_scriptRunner && m_scriptRunner->isRunning()) {
        m_scriptRunner->resume();
        m_scriptRunner->wait(3000);
    }
}

// ---------------------------------------------------------------------------
// Creates the menu bar with File, Edit, Script, and Help menus.
// All menu items use lambda or slot connections with standard keyboard shortcuts.
// ---------------------------------------------------------------------------
void MainWindow::createMenus()
{
    // File menu: New, Open File, Open Folder, Save, Save as HTML, Close Tab, Exit
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&New", this, &MainWindow::newFile, QKeySequence::New);
    fileMenu->addAction("&Open File", this, &MainWindow::openFile, QKeySequence::Open);
    fileMenu->addAction("Open &Folder", this, &MainWindow::openFolder);
    fileMenu->addAction("&Save", this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addAction("Save as &HTML", this, &MainWindow::saveFileAsHTML);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close Tab", this, [this]() {
        int idx = m_tabWidget->currentIndex();
        if (idx >= 0) closeTab(idx);
    }, QKeySequence("Ctrl+F4"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    // Edit menu: Format Code, Toggle Comment, Find, Find Next, Find Previous
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("&Format Code", this, &MainWindow::formatCode, QKeySequence("Ctrl+I"));
    editMenu->addAction("Toggle &Comment", this, &MainWindow::toggleComment, QKeySequence("Ctrl+/"));
    editMenu->addSeparator();
    editMenu->addAction("&Find", this, &MainWindow::showFindBar, QKeySequence::Find);
    editMenu->addAction("Find &Next", this, &MainWindow::findNext, QKeySequence("F3"));
    editMenu->addAction("Find &Previous", this, &MainWindow::findPrev, QKeySequence("Shift+F3"));

    // Script menu: Run (F5 also resumes from breakpoint), Stop, View Debug Panel
    QMenu *scriptMenu = menuBar()->addMenu("&Script");
    m_runAction = scriptMenu->addAction("&Run", this, &MainWindow::runScript, QKeySequence("F5"));
    m_stopAction = scriptMenu->addAction("&Stop", this, &MainWindow::stopScript);
    m_stopAction->setEnabled(false);
    scriptMenu->addAction("View &Debug Panel", this, &MainWindow::viewDebugPanel, QKeySequence("F6"));

    // Help menu: NSP Online, NSP Syntax, About
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("NSP &Online", this, &MainWindow::helpOnline);
    helpMenu->addAction("NSP &Syntax", this, &MainWindow::helpSyntax);
    helpMenu->addSeparator();
    helpMenu->addAction("&About", this, &MainWindow::about);
}

// ---------------------------------------------------------------------------
// Returns a green play-triangle QPixmap used as the Run button icon.
// Defined as an XPM string to avoid needing a separate resource file for SVG/PNG.
// ---------------------------------------------------------------------------
static QPixmap GetRunIcon()
{
    // XPM definition: 22x22 pixels, 3 colors (transparent, dark green, light green)
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

// Returns a red square QPixmap used as the Stop button icon.
static QPixmap GetStopIcon()
{
    // XPM definition: 22x22 pixels, 2 colors (transparent, red)
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

// ---------------------------------------------------------------------------
// Creates the icon-only toolbar with New, Open, Save, and Run buttons.
// Uses QStyle::StandardPixmap for the first three, and the custom XPM for Run.
// ---------------------------------------------------------------------------
void MainWindow::createToolBar()
{
    QToolBar *toolbar = addToolBar("Main");
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(22, 22));
    QStyle *s = style();

    toolbar->addAction(s->standardIcon(QStyle::SP_FileIcon), "New", this, &MainWindow::newFile);
    toolbar->addAction(s->standardIcon(QStyle::SP_DirOpenIcon), "Open", this, &MainWindow::openFile);
    toolbar->addAction(s->standardIcon(QStyle::SP_DialogSaveButton), "Save", this, &MainWindow::saveFile);
    toolbar->addSeparator();
    QAction *runAction = toolbar->addAction("Run", this, &MainWindow::runScript);
    runAction->setIcon(QIcon(GetRunIcon()));
    m_toolbarStopAction = toolbar->addAction("Stop", this, &MainWindow::stopScript);
    m_toolbarStopAction->setIcon(QIcon(GetStopIcon()));
    m_toolbarStopAction->setEnabled(false);
}

// ---------------------------------------------------------------------------
// Creates the status bar with a stretchy status message label on the left
// and a permanent line-number label on the right.
// ---------------------------------------------------------------------------
void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    m_lineLabel = new QLabel("Line: 1");
    statusBar()->addWidget(m_statusLabel, 1);          // stretches to fill
    statusBar()->addPermanentWidget(m_lineLabel);      // fixed width on the right
}

// ---------------------------------------------------------------------------
// Creates a new tab containing a vertical QSplitter with a CodeEditor and
// an OutputPanel. When isNsp is true (the default), the editor gets a syntax
// highlighter and an NspCompleter for autocomplete.
//
// IMPORTANT: Tab properties (fileName, isNspTemplate) must be set BEFORE
// addTab() because the currentChanged signal fires during addTab and
// switchTab() reads these properties.
// ---------------------------------------------------------------------------
QWidget *MainWindow::createTab(bool isNsp)
{
    // Vertical splitter: editor on top, output on bottom (7:1 stretch ratio)
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    CodeEditor *editor = new CodeEditor(splitter);
    OutputPanel *output = new OutputPanel(splitter);

    QFont font("Courier New", 10);
    editor->setFont(font);

    // For .ns and .nsp files, set up syntax highlighting and autocomplete
    if (isNsp) {
        m_completer = new NspCompleter(editor, this);
        editor->setCompleter(m_completer);
        editor->setHighlighter(new NspSyntaxHighlighter(editor->document()));
    }

    // Overlay find bar on top of the editor (positioned top-right)
    FindBar *findBar = new FindBar(editor, editor);
    findBar->move(editor->width() - findBar->width() - 10, 0);

    splitter->addWidget(editor);
    splitter->addWidget(output);
    splitter->setStretchFactor(0, 7);   // editor gets ~87.5%
    splitter->setStretchFactor(1, 1);   // output gets ~12.5%

    // Update the line number in the status bar when the cursor moves
    connect(editor, &CodeEditor::cursorPositionChanged, this, [this, editor]() {
        int line = editor->textCursor().blockNumber() + 1;
        m_lineLabel->setText(QString("Line: %1").arg(line));
    });

    // Toggle the * prefix on the tab label when the document's modified state changes
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, splitter](bool) {
        updateTabText(splitter);
    });

    return splitter;
}

// ---------------------------------------------------------------------------
// Configures the syntax highlighter mode based on the file extension.
// .nsp files use template mode (HTML + <?nsp ?> blocks); .ns files use pure script mode.
// ---------------------------------------------------------------------------
void MainWindow::configureHighlighter(CodeEditor *editor, const QString &fileName)
{
    if (!editor || !editor->highlighter()) return;

    QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "nsp")
        editor->highlighter()->setNspMode(true);
    else
        editor->highlighter()->setNspMode(false);
}

// ---------------------------------------------------------------------------
// Updates a tab's label to show the filename (or "Untitled") and prefixes
// it with * if the document has unsaved modifications.
// ---------------------------------------------------------------------------
void MainWindow::updateTabText(QWidget *tab)
{
    int idx = m_tabWidget->indexOf(tab);
    if (idx < 0) return;

    QString fileName = tab->property("fileName").toString();
    QString name = fileName.isEmpty() ? QString("Untitled") : QFileInfo(fileName).fileName();

    // Prepend * if the editor's document has been modified since last save
    QSplitter *splitter = qobject_cast<QSplitter *>(tab);
    CodeEditor *editor = splitter ? qobject_cast<CodeEditor *>(splitter->widget(0)) : nullptr;
    if (editor && editor->document()->isModified())
        name = "*" + name;

    m_tabWidget->setTabText(idx, name);
}

// ---------------------------------------------------------------------------
// Returns the CodeEditor from the currently active tab, or nullptr if none.
// ---------------------------------------------------------------------------
CodeEditor *MainWindow::activeCodeEditor() const
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (!widget) return nullptr;
    QSplitter *splitter = qobject_cast<QSplitter *>(widget);
    if (!splitter) return nullptr;
    return qobject_cast<CodeEditor *>(splitter->widget(0));
}

// ---------------------------------------------------------------------------
// Returns the OutputPanel from the currently active tab, or nullptr if none.
// ---------------------------------------------------------------------------
OutputPanel *MainWindow::activeOutputPanel() const
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (!widget) return nullptr;
    QSplitter *splitter = qobject_cast<QSplitter *>(widget);
    if (!splitter) return nullptr;
    return qobject_cast<OutputPanel *>(splitter->widget(1));
}

// ---------------------------------------------------------------------------
// Returns the file path stored as a property on the current tab widget,
// or an empty string for untitled tabs.
// ---------------------------------------------------------------------------
QString MainWindow::activeFileName() const
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (!widget) return QString();
    return widget->property("fileName").toString();
}

// ---------------------------------------------------------------------------
// Creates a new untitled .ns tab, configures its highlighter in pure script
// mode, and makes it the active tab.
// ---------------------------------------------------------------------------
void MainWindow::newFile()
{
    QWidget *tab = createTab();
    // Set properties BEFORE addTab — currentChanged fires during addTab and
    // switchTab() needs these values to initialize the tab correctly
    tab->setProperty("fileName", QString());
    tab->setProperty("isNspTemplate", false);

    int idx = m_tabWidget->addTab(tab, "Untitled");
    m_tabWidget->setCurrentIndex(idx);

    // Default to .ns pure script highlighting for new untitled tabs
    CodeEditor *editor = activeCodeEditor();
    if (editor) {
        configureHighlighter(editor, "untitled.ns");
    }

    setWindowTitle("NSP Editor - [Untitled]");
}

// ---------------------------------------------------------------------------
// Shows the file open dialog filtered for .ns/.nsp files, saves the last-used
// directory, and delegates to loadFile().
// ---------------------------------------------------------------------------
void MainWindow::openFile()
{
    QString lastDir = Settings::loadLastDir();
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open NSP Script", lastDir, "NSP Scripts (*.ns *.nsp);;All Files (*)");
    if (fileName.isEmpty()) return;

    Settings::saveLastDir(QFileInfo(fileName).absolutePath());
    loadFile(fileName);
}

// ---------------------------------------------------------------------------
// Shows a folder picker dialog, updates the file browser root, and makes it visible.
// ---------------------------------------------------------------------------
void MainWindow::openFolder()
{
    QString lastDir = Settings::loadLastDir();
    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", lastDir);
    if (folder.isEmpty()) return;

    Settings::saveLastDir(folder);
    m_fileBrowser->setRootPath(folder);
    m_fileBrowser->setVisible(true);
}

// Delegate — opens a file from the file browser by double-clicking
void MainWindow::openFileFromBrowser(const QString &filePath)
{
    loadFile(filePath);
}

// ---------------------------------------------------------------------------
// Opens a file into a new tab. If the file is already open in an existing
// tab, switches to that tab instead. Reads the file content, creates a tab
// with appropriate highlighter/autocomplete settings, and displays it.
// ---------------------------------------------------------------------------
void MainWindow::loadFile(const QString &fileName)
{
    // If the file is already open in a tab, just switch to it
    for (int i = 0; i < m_tabWidget->count(); i++) {
        QWidget *w = m_tabWidget->widget(i);
        if (w && w->property("fileName").toString() == fileName) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // Read file content from disk
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString content = QTextStream(&file).readAll();
    file.close();

    // Determine file type — only .ns and .nsp files get highlighting and autocomplete
    QString ext = QFileInfo(fileName).suffix().toLower();
    bool isNsp = (ext == "ns" || ext == "nsp");

    // Create tab with appropriate features; set properties BEFORE addTab
    QWidget *tab = createTab(isNsp);
    tab->setProperty("fileName", fileName);
    tab->setProperty("isNspTemplate", ext == "nsp");

    int idx = m_tabWidget->addTab(tab, QFileInfo(fileName).fileName());
    m_tabWidget->setCurrentIndex(idx);

    // Set editor content and mark it as unmodified
    CodeEditor *editor = activeCodeEditor();
    if (editor) {
        editor->setPlainText(content);
        editor->document()->setModified(false);
        configureHighlighter(editor, fileName);
    }

    setWindowTitle(QString("NSP Editor - [%1]").arg(QFileInfo(fileName).fileName()));

    // Show a brief message in the output panel and status bar
    OutputPanel *output = activeOutputPanel();
    if (output)
        output->appendOutput(QString("Loaded %1\n").arg(fileName));

    setStatus(QString("Loaded %1").arg(fileName));
}

// ---------------------------------------------------------------------------
// Saves the active editor's content to disk. If the file has no path yet
// (untitled), prompts for a save location. Clears the document's modified
// flag on success, which triggers updateTabText to remove the * prefix.
// ---------------------------------------------------------------------------
void MainWindow::saveFile()
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return;

    QString fileName = activeFileName();
    if (fileName.isEmpty()) {
        // Untitled file — prompt the user for a save path
        QString lastDir = Settings::loadLastDir();
        fileName = QFileDialog::getSaveFileName(this,
            "Save NSP Script", lastDir, "NSP Scripts (*.ns);;All Files (*)");
        if (fileName.isEmpty()) return;
        Settings::saveLastDir(QFileInfo(fileName).absolutePath());
        m_tabWidget->currentWidget()->setProperty("fileName", fileName);
        updateTabText(m_tabWidget->currentWidget());
        setWindowTitle(QString("NSP Editor - [%1]").arg(QFileInfo(fileName).fileName()));
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream(&file) << editor->toPlainText();
    file.close();
    editor->document()->setModified(false);
    setStatus(QString("Saved %1").arg(fileName));
}

// ---------------------------------------------------------------------------
// Exports the active editor's content as a syntax-highlighted HTML file.
// Walks each QTextBlock, extracts FormatRange entries from the layout, and
// generates inline <span style="color:..."> tags matching the current theme
// (light or dark). Uses background/foreground colors appropriate for the
// detected theme mode.
// ---------------------------------------------------------------------------
void MainWindow::saveFileAsHTML()
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return;

    QString currentFile = activeFileName();
    // Construct a default HTML filename based on the current script's name
    QString defaultName;
    if (!currentFile.isEmpty()) {
        QFileInfo fi(currentFile);
        defaultName = fi.absolutePath() + "/" + fi.completeBaseName() + ".html";
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Save as HTML", defaultName, "HTML Files (*.html)");
    if (fileName.isEmpty()) return;

    // Choose background/foreground colors based on the current theme (light or dark)
    bool isDark = editor->isDark();
    QColor bgColor = isDark ? QColor(30, 30, 30) : QColor(255, 255, 255);
    QColor fgColor = isDark ? QColor(220, 220, 220) : QColor(0, 0, 0);

    // Retrieve the highlighter's format objects for identifying token types
    QTextCharFormat commentFormat, stringFormat, keywordFormat, reservedFormat, numberFormat, operatorFormat, memberFormat, tagFormat;
    if (editor->highlighter()) {
        commentFormat = editor->highlighter()->commentFormat();
        stringFormat = editor->highlighter()->stringFormat();
        keywordFormat = editor->highlighter()->keywordFormat();
        reservedFormat = editor->highlighter()->reservedFormat();
        numberFormat = editor->highlighter()->numberFormat();
        operatorFormat = editor->highlighter()->operatorFormat();
        memberFormat = editor->highlighter()->memberFormat();
        tagFormat = editor->highlighter()->tagFormat();
    }

    // Converts a QTextCharFormat and text snippet into an HTML <span> element
    // with inline style for color, italic (comments), and bold (keywords)
    auto formatToSpan = [](const QTextCharFormat &fmt, const QString &text) -> QString {
        QString escaped = text.toHtmlEscaped();
        QColor color = fmt.foreground().color();
        if (!color.isValid() || color == Qt::black)
            return escaped;
        QStringList styles;
        styles << QString("color:%1").arg(color.name());
        if (fmt.fontItalic())
            styles << "font-style:italic";
        if (fmt.fontWeight() >= QFont::Bold)
            styles << "font-weight:bold";
        return QString("<span style=\"%1\">%2</span>").arg(styles.join(";"), escaped);
    };

    // Maps a QTextCharFormat back to its token category name (for debug / fallback matching)
    auto formatName = [&](const QTextCharFormat &fmt) -> QString {
        if (fmt == commentFormat) return "comment";
        if (fmt == stringFormat) return "string";
        if (fmt == keywordFormat) return "keyword";
        if (fmt == reservedFormat) return "reserved";
        if (fmt == numberFormat) return "number";
        if (fmt == operatorFormat) return "operator";
        if (fmt == memberFormat) return "member";
        if (fmt == tagFormat) return "tag";
        return QString();
    };

    // Walk each block in the document and generate HTML from its format ranges
    QStringList lines;
    QTextDocument *doc = editor->document();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QString lineHtml;
        QString text = block.text();
        QTextLayout *layout = block.layout();
        QList<QTextLayout::FormatRange> formats = layout->formats();

        if (formats.isEmpty() || !editor->highlighter()) {
            // No syntax highlighting — just escape the plain text
            lineHtml = text.toHtmlEscaped();
        } else {
            // Process format ranges: unformatted gaps stay plain, formatted
            // ranges get <span> tags (unless the color matches default fg)
            int pos = 0;
            for (const QTextLayout::FormatRange &fr : formats) {
                if (fr.start > pos) {
                    lineHtml += text.mid(pos, fr.start - pos).toHtmlEscaped();
                }
                QString name = formatName(fr.format);
                if (name.isEmpty() || fr.format.foreground().color() == fgColor) {
                    // No distinct color — render as plain text
                    lineHtml += text.mid(fr.start, fr.length).toHtmlEscaped();
                } else {
                    lineHtml += formatToSpan(fr.format, text.mid(fr.start, fr.length));
                }
                pos = fr.start + fr.length;
            }
            // Remaining text after the last format range
            if (pos < text.length()) {
                lineHtml += text.mid(pos).toHtmlEscaped();
            }
        }
        lines.append(lineHtml);
    }

    // Wrap in a minimal HTML document with monospace font and theme-matching colors
    QString html = QString("<!DOCTYPE html>\n<html>\n<head>\n<style>\n"
        "body { background-color: %1; color: %2; font-family: monospace; font-size: 14px; }\n"
        "pre { margin: 0; padding: 0; }\n"
        "</style>\n</head>\n<body>\n<pre>%3</pre>\n</body>\n</html>")
        .arg(bgColor.name(), fgColor.name(), lines.join("\n"));

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&file) << html;
        file.close();
    }
}

// ---------------------------------------------------------------------------
// Closes the tab at the given index and deletes its widget. If no tabs
// remain, creates a new untitled tab so the window is never empty.
// ---------------------------------------------------------------------------
void MainWindow::closeTab(int index)
{
    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete widget;

    if (m_tabWidget->count() == 0)
        newFile();
}

// ---------------------------------------------------------------------------
// Runs the active script. If the script runner is paused at a breakpoint,
// resumes instead. .nsp template files cannot be run. Clears the output
// panel before starting, sets the working directory to the script's path,
// and creates a new ScriptRunner thread.
// ---------------------------------------------------------------------------
void MainWindow::runScript()
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return;

    // .nsp template files are not executable scripts
    QWidget *widget = m_tabWidget->currentWidget();
    if (widget && widget->property("isNspTemplate").toBool()) {
        setStatus("Cannot run .nsp template files");
        return;
    }

    // If the script runner is paused at a breakpoint, F5 resumes execution
    if (m_scriptRunner && m_scriptRunner->isRunning()) {
        m_scriptRunner->resume();
        return;
    }

    clearOutput();

    QString scriptText = editor->toPlainText();
    if (scriptText.isEmpty()) return;

    // Set working directory to the script's location so relative paths work
    QString srcFile = activeFileName();
    if (!srcFile.isEmpty()) {
        QDir::setCurrent(QFileInfo(srcFile).absolutePath());
    }

    // Create and start a new script runner thread
    m_scriptRunner = new ScriptRunner(scriptText, srcFile, this);
    connect(m_scriptRunner, &ScriptRunner::outputReady, this, &MainWindow::onOutputReady);
    connect(m_scriptRunner, &ScriptRunner::scriptFinished, this, &MainWindow::onScriptFinished);
    connect(m_scriptRunner, &ScriptRunner::breakpointHit, this, &MainWindow::onBreakpointHit);
    m_scriptRunner->start();

    // Update Run button text to Resume while script is running
    m_runAction->setText("&Resume");
    m_debugPanel->setRunning(true);
    m_stopAction->setEnabled(true);
    m_toolbarStopAction->setEnabled(true);
}

// ---------------------------------------------------------------------------
// Stops a running script. Called from the debug panel Stop button.
// ---------------------------------------------------------------------------
void MainWindow::stopScript()
{
    if (m_scriptRunner && m_scriptRunner->isRunning()) {
        m_scriptRunner->stop();
        delete m_scriptRunner;
        m_scriptRunner = nullptr;
        setStatus("Script stopped");
        m_runAction->setText("&Run");
        m_debugPanel->setRunning(false);
        m_stopAction->setEnabled(false);
        m_toolbarStopAction->setEnabled(false);
    }
}

// ---------------------------------------------------------------------------
// Toggles the debug panel visibility. Always accessible via F6.
// If a script is running, populates the tree with the current NSP state.
// ---------------------------------------------------------------------------
void MainWindow::viewDebugPanel()
{
    if (m_scriptRunner && m_scriptRunner->isRunning())
        m_debugPanel->setNspState(m_scriptRunner->nspState());

    m_debugPanel->setVisible(!m_debugPanel->isVisible());
}

// Shows the inline find bar on the active editor, pre-filling with selected text
void MainWindow::showFindBar()
{
    FindBar *bar = activeFindBar();
    if (bar) bar->showFind();
}

// Advances to the next search match in the active find bar.
// Shows the find bar first if it's not currently visible.
void MainWindow::findNext()
{
    FindBar *bar = activeFindBar();
    if (bar) {
        if (!bar->isVisible()) bar->showFind();
        bar->findNext();
    }
}

// Advances to the previous search match in the active find bar.
// Shows the find bar first if it's not currently visible.
void MainWindow::findPrev()
{
    FindBar *bar = activeFindBar();
    if (bar) {
        if (!bar->isVisible()) bar->showFind();
        bar->findPrev();
    }
}

// ---------------------------------------------------------------------------
// Locates the FindBar child widget in the active CodeEditor's hierarchy.
// Each CodeEditor has its own FindBar instance created in createTab().
// ---------------------------------------------------------------------------
FindBar *MainWindow::activeFindBar() const
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return nullptr;
    return editor->findChild<FindBar *>();
}

// Opens the NSP project website in the default browser
void MainWindow::helpOnline()
{
    QDesktopServices::openUrl(QUrl("https://nulllogic.ca/"));
}

// Opens the NSP syntax reference page in the default browser
void MainWindow::helpSyntax()
{
    QDesktopServices::openUrl(QUrl("https://nulllogic.ca/nsp/syntax.html"));
}

// Shows the About dialog with version and copyright information
void MainWindow::about()
{
    QMessageBox::about(this, "About NSP Editor",
        "NSP Editor v0.9.4\n\n"
        "NESLA NullLogic Embedded Scripting Language\n"
        "Copyright (C) 2007-2023 Dan Cahill\n\n"
        "A Qt-based editor for NSP scripts.");
}

// Appends script output (stdout) to the active tab's output panel
void MainWindow::onOutputReady(const QString &text)
{
    OutputPanel *output = activeOutputPanel();
    if (output)
        output->appendOutput(text);
}

// ---------------------------------------------------------------------------
// Called when a script finishes execution. Updates the status bar with
// "filename — done" or "filename — finished with errors", and appends any
// error output. Cleans up the ScriptRunner thread.
// ---------------------------------------------------------------------------
void MainWindow::onScriptFinished(bool error, const QString &errbuf)
{
    OutputPanel *output = activeOutputPanel();
    if (error && output) {
        output->appendOutput(QString("\nError: %1").arg(errbuf));
    }
    setStatus(error ? QString("%1 — finished with errors").arg(QFileInfo(m_scriptRunner->filename()).fileName()) : QString("%1 — done").arg(QFileInfo(m_scriptRunner->filename()).fileName()));
    m_scriptRunner->deleteLater();
    m_scriptRunner = nullptr;

    // Reset Run button text
    m_runAction->setText("&Run");
    m_debugPanel->setRunning(false);
    m_stopAction->setEnabled(false);
    m_toolbarStopAction->setEnabled(false);
}

// ---------------------------------------------------------------------------
// Called when debug.break() is hit inside a script. Opens the debug panel,
// populates it with the current NSP state, and shows a status message
// indicating the script is paused at a breakpoint.
void MainWindow::onBreakpointHit()
{
    m_debugPanel->setNspState(m_scriptRunner->nspState());
    m_debugPanel->setVisible(true);
    setStatus("Paused at breakpoint — press F5 to continue, F6 to view debug panel");
}

// ---------------------------------------------------------------------------
// Called when the active tab changes. Updates the window title and re-activates
// the autocomplete/highlighter for the newly selected tab. If an editor tab
// lacks a completer but is an .ns/.nsp file, creates one for it.
// ---------------------------------------------------------------------------
void MainWindow::switchTab(int index)
{
    if (index < 0) return;
    QWidget *widget = m_tabWidget->widget(index);
    if (!widget) return;

    QString fileName = widget->property("fileName").toString();
    if (fileName.isEmpty())
        fileName = "Untitled";

    setWindowTitle(QString("NSP Editor - [%1]").arg(QFileInfo(fileName).fileName()));

    // Restore the completer and highlighter mode for the newly active tab
    QSplitter *splitter = qobject_cast<QSplitter *>(widget);
    if (splitter) {
        CodeEditor *editor = qobject_cast<CodeEditor *>(splitter->widget(0));
        if (editor) {
            m_completer = editor->completer();
            if (!m_completer) {
                // Create a completer for .ns/.nsp files that don't have one yet
                QString ext = QFileInfo(fileName).suffix().toLower();
                if (ext == "ns" || ext == "nsp" || fileName == "Untitled") {
                    m_completer = new NspCompleter(editor, this);
                    editor->setCompleter(m_completer);
                }
            }
            configureHighlighter(editor, fileName);
        }
    }
}

// Public convenience — appends text to the current tab's output panel
void MainWindow::appendOutput(const QString &text)
{
    OutputPanel *output = activeOutputPanel();
    if (output)
        output->appendOutput(text);
}

// Public convenience — clears the current tab's output panel
void MainWindow::clearOutput()
{
    OutputPanel *output = activeOutputPanel();
    if (output)
        output->clearOutput();
}

// Sets the status bar's left-side message label
void MainWindow::setStatus(const QString &msg)
{
    m_statusLabel->setText(msg);
}

// ---------------------------------------------------------------------------
// Handles the window close event. Saves window geometry and output panel
// height for next session. If any tab has unsaved changes, prompts the
// user for confirmation. Also resumes any running script thread so it
// can finish before the application exits.
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    Settings::saveWindowGeometry(geometry());
    Settings::saveOutputHeight(m_outputHeight);

    // Check all tabs for unsaved modifications and prompt once
    for (int i = 0; i < m_tabWidget->count(); i++) {
        QWidget *widget = m_tabWidget->widget(i);
        QSplitter *splitter = qobject_cast<QSplitter *>(widget);
        if (splitter) {
            CodeEditor *editor = qobject_cast<CodeEditor *>(splitter->widget(0));
            if (editor && editor->document()->isModified()) {
                int result = QMessageBox::question(this, "Confirm",
                    "There are unsaved changes. Close anyway?",
                    QMessageBox::Yes | QMessageBox::No);
                if (result == QMessageBox::No) {
                    event->ignore();
                    return;
                }
                break;  // Only prompt once, even if multiple tabs are modified
            }
        }
    }

    // Resume any paused script so the thread can exit cleanly
    if (m_scriptRunner && m_scriptRunner->isRunning()) {
        m_scriptRunner->resume();
        m_scriptRunner->wait(3000);
    }

    event->accept();
}

// ---------------------------------------------------------------------------
// Formats (re-indents) the active editor's code using NspFormatter.
// Only works on .ns and .nsp files — returns early for untitled or other
// extensions. Uses .nsp template mode for .nsp files and .ns pure mode
// for .ns files. Preserves the cursor position by line/column rather than
// character offset, since formatting changes character counts.
// The entire document is replaced as a single undo block.
// ---------------------------------------------------------------------------
void MainWindow::formatCode()
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return;

    QString fileName = activeFileName();
    if (fileName.isEmpty()) return;

    // Only format NSP script files
    QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext != "ns" && ext != "nsp") return;

    // Save cursor position by line and column (not character offset)
    QTextCursor cursor = editor->textCursor();
    int blockNumber = cursor.blockNumber();
    int columnInBlock = cursor.position() - cursor.block().position();

    // Format using the appropriate mode
    QString formatted;
    if (ext == "nsp")
        formatted = NspFormatter::formatNsp(editor->toPlainText());
    else
        formatted = NspFormatter::format(editor->toPlainText());

    // Replace entire document content as a single undo block
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(formatted);
    cursor.endEditBlock();

    // Restore cursor position by line/column
    QTextBlock block = editor->document()->findBlockByNumber(qMin(blockNumber, editor->document()->blockCount() - 1));
    if (block.isValid()) {
        int newPos = qMin(block.position() + columnInBlock, block.position() + block.length() - 1);
        cursor.setPosition(newPos);
    }
    editor->setTextCursor(cursor);
}

// ---------------------------------------------------------------------------
// Toggles // comment prefix on the currently selected lines.
// - If all selected lines already start with //, removes the // prefix
// - Otherwise, inserts // at the beginning of each selected line
// - When the cursor is at column 0 of the end line (trailing selection),
//   that line is excluded from the range to avoid commenting the line
//   after the user's visible selection
// - Operates in reverse order (bottom to top) when inserting to preserve
//   character positions, and the final selection spans all affected lines
// ---------------------------------------------------------------------------
void MainWindow::toggleComment()
{
    CodeEditor *editor = activeCodeEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();

    // Determine the range of line numbers to operate on
    int startLine = editor->document()->findBlock(cursor.selectionStart()).blockNumber();
    int endLine = editor->document()->findBlock(cursor.selectionEnd()).blockNumber();

    // If the selection ends exactly at column 0 of the next line, exclude
    // that trailing line — the user didn't intend to include it
    if (cursor.hasSelection()) {
        QTextBlock endBlock = editor->document()->findBlock(cursor.selectionEnd());
        if (cursor.selectionEnd() == endBlock.position())
            endLine--;
    }

    // Check whether all non-empty lines in the range are already commented
    bool allCommented = true;
    for (int i = startLine; i <= endLine; i++) {
        QTextBlock block = editor->document()->findBlockByNumber(i);
        QString text = block.text();
        if (!text.isEmpty() && !text.trimmed().startsWith("//"))
            allCommented = false;
    }

    // Process lines in reverse order so character positions aren't disrupted
    cursor.beginEditBlock();
    for (int i = endLine; i >= startLine; i--) {
        QTextBlock block = editor->document()->findBlockByNumber(i);
        QString text = block.text();
        if (text.isEmpty()) continue;

        cursor.setPosition(block.position());
        if (allCommented) {
            // Uncomment: remove // prefix (may be preceded by spaces for indented comments)
            if (text.startsWith("//")) {
                cursor.setPosition(block.position());
                cursor.setPosition(block.position() + 2, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            } else {
                // Handle indented // (e.g., "  //comment")
                int idx = 0;
                while (idx < text.length() && text[idx] == ' ')
                    idx++;
                if (idx >= 2 && text.mid(idx - 2, 2) == "//") {
                    cursor.setPosition(block.position() + idx - 2);
                    cursor.setPosition(block.position() + idx, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                }
            }
        } else {
            // Comment: insert // at the beginning of the line
            cursor.insertText("//");
        }
    }
    cursor.endEditBlock();

    // Re-select the full range of affected lines so the user can see the result
    QTextBlock startBlock = editor->document()->findBlockByNumber(startLine);
    QTextBlock endBlock = editor->document()->findBlockByNumber(endLine);
    QTextCursor selCursor(editor->document());
    selCursor.setPosition(startBlock.position());
    selCursor.setPosition(endBlock.position() + endBlock.length() - 1, QTextCursor::KeepAnchor);
    editor->setTextCursor(selCursor);
}