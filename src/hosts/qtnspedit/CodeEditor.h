#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QSet>

class NspCompleter;
class NspSyntaxHighlighter;

class SourceMap;

// CodeEditor - main text editor widget with syntax highlighting, code folding,
// autocomplete, zoom, tooltips, and an integrated source map minimap.
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor();

    // Line number gutter painting and width calculation
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    // Mouse interaction in the line number area (fold indicator clicks, hover)
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    void lineNumberAreaMouseMoveEvent(QMouseEvent *event);
    void lineNumberAreaLeaveEvent(QEvent *event);

    // Extracts the identifier (possibly dotted, e.g. "io.print") at cursorPos
    QString getLabel(int cursorPos, bool readPastCursor) const;
    void setCompleter(NspCompleter *completer);
    void setHighlighter(NspSyntaxHighlighter *highlighter);
    NspCompleter *completer() const { return m_completer; }
    NspSyntaxHighlighter *highlighter() const { return m_highlighter; }
    bool isDark() const { return m_isDark; }

signals:
    // Emitted when a .ns/.nsp file is drag-dropped onto the editor
    void fileDropped(const QString &fileName);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override; // Tooltips for syntax elements
    void changeEvent(QEvent *event) override;      // Detect OS theme changes

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *m_lineNumberArea;
    NspCompleter *m_completer;
    NspSyntaxHighlighter *m_highlighter;
    SourceMap *m_sourceMap;          // Minimap widget on the right edge
    QSet<int> m_foldedBlocks;        // Set of block numbers that are currently collapsed
    bool m_isDark;                   // Cached dark mode flag
    bool m_foldHover;                // True when mouse is over the fold indicator column
    qreal m_zoomLevel;               // Accumulated zoom delta (0 = default 10pt)

    static bool isDarkTheme();       // Checks OS palette for dark/light theme

    // Code folding helpers
    int findMatchingBrace(const QTextBlock &startBlock) const;
    bool isFoldable(const QTextBlock &block) const;
    void toggleFold(int blockNumber);
    void zoom(int delta);            // Adjust font size by delta points

    // LineNumberArea - thin widget rendered by CodeEditor for line numbers + fold indicators
    class LineNumberArea : public QWidget {
    public:
        LineNumberArea(CodeEditor *editor) : QWidget(editor), m_editor(editor) { setMouseTracking(true); }
        QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }
    protected:
        void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }
        void mousePressEvent(QMouseEvent *event) override { m_editor->lineNumberAreaMousePressEvent(event); }
        void mouseMoveEvent(QMouseEvent *event) override { m_editor->lineNumberAreaMouseMoveEvent(event); }
        void leaveEvent(QEvent *event) override { m_editor->lineNumberAreaLeaveEvent(event); }
    private:
        CodeEditor *m_editor;
    };

    friend class LineNumberArea;
    friend class SourceMap;
};

// SourceMap - minimap widget showing the entire document compressed vertically.
// Click/drag navigates to the corresponding line. Syntax colors are blended
// with the editor background to approximate the visual appearance at scale.
// Leading whitespace is left as background; trailing whitespace after code
// inherits the last syntax color on that line for visual continuity.
class SourceMap : public QWidget {
    Q_OBJECT
public:
    explicit SourceMap(CodeEditor *editor, QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    CodeEditor *m_editor;
    bool m_dragging;                 // True during click-drag navigation
    int documentHeight() const;
    int visibleFrom() const;         // Y pixel of first visible line in map coordinates
    int visibleTo() const;           // Y pixel of last visible line in map coordinates
};

#endif // CODEEDITOR_H