#include "CodeEditor.h"
#include "NspSyntaxHighlighter.h"
#include "NspCompleter.h"
#include "NspNameSpace.h"
#include "FindBar.h"

#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QToolTip>
#include <QScrollBar>
#include <QTimer>
#include <QApplication>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_completer(nullptr)
    , m_highlighter(nullptr)
    , m_isDark(isDarkTheme())
    , m_foldHover(false)
    , m_zoomLevel(0)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_sourceMap = new SourceMap(this, this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(32);
    setAcceptDrops(true);
    setMouseTracking(true); // Required for hover tooltips

    // Source map needs to repaint when the viewport scrolls or document changes
    connect(verticalScrollBar(), &QScrollBar::valueChanged, m_sourceMap, qOverload<>(&QWidget::update));
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, m_sourceMap, qOverload<>(&QWidget::update));
    connect(this, &CodeEditor::textChanged, m_sourceMap, qOverload<>(&QWidget::update));
    connect(this, &CodeEditor::blockCountChanged, m_sourceMap, qOverload<>(&QWidget::update));

    updateLineNumberAreaWidth(0);
}

CodeEditor::~CodeEditor()
{
}

// Detect dark/light theme from the OS palette
bool CodeEditor::isDarkTheme()
{
    return QApplication::palette().color(QPalette::Base).value() < 128;
}

// Handle OS theme changes - switch highlighter colors and repaint
void CodeEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        bool dark = isDarkTheme();
        if (dark != m_isDark) {
            m_isDark = dark;
            if (m_highlighter)
                m_highlighter->refreshDarkMode();
            m_lineNumberArea->update();
            m_sourceMap->update();
            viewport()->update();
        }
    }
}

// Zoom in/out by delta points, clamped to 4-48pt range
void CodeEditor::zoom(int delta)
{
    int newSize = font().pointSize() + delta;
    if (newSize < 4 || newSize > 48) return;
    m_zoomLevel += delta;
    QFont f = font();
    f.setPointSize(newSize);
    setFont(f);
    setTabStopDistance(32.0 * newSize / 10.0);
    updateLineNumberAreaWidth(0);
    viewport()->update();
    m_sourceMap->update();
}

// Ctrl+Scroll zooms; passes other scroll events to base class
void CodeEditor::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        zoom(e->angleDelta().y() > 0 ? 1 : -1);
        e->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(e);
}

// Line number area width = margin + digit width * digit count + fold indicator column
int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + fontMetrics().height();
    return space;
}

// Paint the line number gutter: line numbers on the left, fold indicators on the right.
// Fold indicators: collapsed blocks show ▶ (always), expanded blocks show ▼ (on hover only).
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_isDark ? QColor(50, 50, 54) : QColor(240, 240, 240));

    int foldSize = fontMetrics().height();
    int numberWidth = lineNumberAreaWidth() - foldSize;

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            // Draw line number right-aligned in the number portion
            QString number = QString::number(blockNumber + 1);
            painter.setPen(m_isDark ? QColor(160, 160, 160) : QColor(120, 120, 120));
            painter.drawText(0, top, numberWidth, fontMetrics().height(),
                Qt::AlignRight, number);
        }

        // Draw fold indicator if block is foldable
        if (isFoldable(block)) {
            bool folded = m_foldedBlocks.contains(block.blockNumber());
            bool visible = folded || m_foldHover;
            if (visible) {
                int sz = foldSize / 2 + 2;
                int cx = numberWidth + foldSize / 2;
                int cy = top + foldSize / 2;
                painter.setPen(m_isDark ? QColor(180, 180, 180) : QColor(80, 80, 80));
                if (folded) {
                    // ▶ collapsed arrow
                    painter.drawLine(cx - sz / 2, cy - sz / 2, cx + sz / 2, cy);
                    painter.drawLine(cx - sz / 2, cy + sz / 2, cx + sz / 2, cy);
                } else {
                    // ▼ expanded arrow
                    painter.drawLine(cx - sz / 2, cy - sz / 2, cx, cy + sz / 2);
                    painter.drawLine(cx + sz / 2, cy - sz / 2, cx, cy + sz / 2);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// Reposition line number area and source map when the editor is resized
void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));

    int hBarHeight = horizontalScrollBar()->isVisible() ? horizontalScrollBar()->height() : 0;
    int smWidth = m_sourceMap->sizeHint().width();
    m_sourceMap->setGeometry(QRect(cr.right() - smWidth, cr.top(), smWidth, cr.height() - hBarHeight));
}

// Handle keyboard input: autocomplete navigation, zoom shortcuts, and dot-triggered completion
void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    // Pass navigation keys to the completer popup when it's visible
    if (m_completer && m_completer->isPopupVisible()) {
        int key = e->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down ||
            key == Qt::Key_PageUp || key == Qt::Key_PageDown ||
            key == Qt::Key_Home || key == Qt::Key_End) {
            QApplication::sendEvent(m_completer->popup(), e);
            e->accept();
            return;
        }
    }

    // Tab/Shift+Tab: indent/outdent selected lines, or insert tab at cursor
    if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            int startBlock = document()->findBlock(cursor.selectionStart()).blockNumber();
            int endBlock = document()->findBlock(cursor.selectionEnd()).blockNumber();
            // If selection ends at column 0, the last line isn't actually selected
            if (cursor.selectionEnd() == document()->findBlock(cursor.selectionEnd()).position())
                endBlock--;
            cursor.beginEditBlock();
            for (int i = startBlock; i <= endBlock; i++) {
                QTextBlock block = document()->findBlockByNumber(i);
                cursor.setPosition(block.position());
                if (e->key() == Qt::Key_Tab) {
                    cursor.insertText("\t");
                } else {
                    QString text = block.text();
                    if (!text.isEmpty() && text[0] == '\t') {
                        cursor.deleteChar();
                    } else if (!text.isEmpty() && text.startsWith("    ")) {
                        cursor.deleteChar();
                        cursor.deleteChar();
                        cursor.deleteChar();
                        cursor.deleteChar();
                    }
                }
            }
            cursor.endEditBlock();
            // Re-select the full range of lines
            QTextBlock firstBlock = document()->findBlockByNumber(startBlock);
            QTextBlock lastBlock = document()->findBlockByNumber(endBlock);
            cursor.setPosition(firstBlock.position());
            cursor.setPosition(lastBlock.position() + lastBlock.length() - 1, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            e->accept();
            return;
        } else if (e->key() == Qt::Key_Tab) {
            cursor.insertText("\t");
            e->accept();
            return;
        }
        // Shift+Tab with no selection: outdent current line
        QTextBlock block = cursor.block();
        QString text = block.text();
        cursor.beginEditBlock();
        if (!text.isEmpty() && text[0] == '\t') {
            cursor.setPosition(block.position());
            cursor.deleteChar();
        } else if (!text.isEmpty() && text.startsWith("    ")) {
            cursor.setPosition(block.position());
            cursor.deleteChar();
            cursor.deleteChar();
            cursor.deleteChar();
            cursor.deleteChar();
        }
        cursor.endEditBlock();
        e->accept();
        return;
    }

    // F3/Shift+F3: find next/previous via the find bar
    if (e->key() == Qt::Key_F3) {
        FindBar *bar = findChild<FindBar *>();
        if (bar) {
            if (!bar->isVisible()) bar->showFind();
            if (e->modifiers() & Qt::ShiftModifier)
                bar->findPrev();
            else
                bar->findNext();
        }
        e->accept();
        return;
    }

    // Zoom shortcuts
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Plus || e->key() == Qt::Key_Equal) {
            zoom(1);
            return;
        } else if (e->key() == Qt::Key_Minus) {
            zoom(-1);
            return;
        } else if (e->key() == Qt::Key_0) {
            m_zoomLevel = 0;
            QFont f = font();
            f.setPointSize(10);
            setFont(f);
            setTabStopDistance(32);
            updateLineNumberAreaWidth(0);
            viewport()->update();
            m_sourceMap->update();
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    // Trigger dot completion
    if (e->key() == Qt::Key_Period) {
        if (m_completer)
            m_completer->popupForDot();
        return;
    }

    // Autocomplete: re-trigger after typing identifier chars or dot
    if (m_completer && e->text().length() == 1) {
        QChar ch = e->text().at(0);
        bool isIdentifierChar = ch.isLetterOrNumber() || ch == '_' || ch == '$';

        if (m_completer->isPopupVisible()) {
            if (isIdentifierChar) {
                QTimer::singleShot(0, this, [this]() {
                    m_completer->triggerCompletion(textCursor().position());
                });
            } else if (ch == '.') {
                QTimer::singleShot(0, this, [this]() {
                    m_completer->popupForDot();
                });
            }
        } else if (isIdentifierChar) {
            // Only auto-trigger after 2+ identifier chars
            QString prefix = m_completer->currentWordPrefix();
            if (prefix.length() >= 2) {
                QTimer::singleShot(0, this, [this]() {
                    m_completer->triggerCompletion(textCursor().position());
                });
            }
        }
    } else if (m_completer && m_completer->isPopupVisible()
               && (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete)) {
        // Hide popup if backspace deletes the prefix
        QTimer::singleShot(0, this, [this]() {
            QString prefix = m_completer->currentWordPrefix();
            if (prefix.isEmpty()) {
                m_completer->popup()->hide();
                m_completer->popup()->infoLabel()->hide();
            } else {
                m_completer->triggerCompletion(textCursor().position());
            }
        });
    }
}

// Accept drag-drops of .ns/.nsp files
void CodeEditor::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

// Emit fileDropped signal for .ns/.nsp file drops
void CodeEditor::dropEvent(QDropEvent *e)
{
    const QList<QUrl> urls = e->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString fname = url.toLocalFile();
        if (fname.endsWith(".ns", Qt::CaseInsensitive) ||
            fname.endsWith(".nsp", Qt::CaseInsensitive))
        {
            emit fileDropped(fname);
        }
    }
}

// Tooltip handler: shows type info for comments, strings, numbers, and namespace identifiers
void CodeEditor::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_highlighter) {
        QToolTip::hideText();
        QPlainTextEdit::mouseMoveEvent(e);
        return;
    }

    int cursorPos = cursorForPosition(e->pos()).position();
    QTextBlock block = document()->findBlock(cursorPos);
    if (!block.isValid()) {
        QToolTip::hideText();
        QPlainTextEdit::mouseMoveEvent(e);
        return;
    }

    // Determine which FormatRange the cursor is over
    int blockOffset = cursorPos - block.position();
    QTextLayout *layout = block.layout();
    QList<QTextLayout::FormatRange> formats = layout->formats();
    for (const QTextLayout::FormatRange &fr : formats) {
        if (blockOffset >= fr.start && blockOffset < fr.start + fr.length) {
            // Show simple type tooltips for syntax primitives
            if (fr.format.foreground().color() == m_highlighter->commentColor()) {
                QToolTip::showText(e->globalPosition().toPoint(), QString("comment"), this);
                QPlainTextEdit::mouseMoveEvent(e);
                return;
            }
            if (fr.format.foreground().color() == m_highlighter->stringColor()) {
                QToolTip::showText(e->globalPosition().toPoint(), QString("string"), this);
                QPlainTextEdit::mouseMoveEvent(e);
                return;
            }
            if (fr.format.foreground().color() == m_highlighter->numberColor()) {
                QToolTip::showText(e->globalPosition().toPoint(), QString("number"), this);
                QPlainTextEdit::mouseMoveEvent(e);
                return;
            }
            // Hide tooltip for keywords/reserved (no tooltip wanted)
            if (fr.format == m_highlighter->keywordFormat() || fr.format == m_highlighter->reservedFormat()) {
                QToolTip::hideText();
                QPlainTextEdit::mouseMoveEvent(e);
                return;
            }
        }
    }

    // Look up identifier in NspNameSpace for detailed tooltips
    QString label = getLabel(cursorPos, true);
    if (label.isEmpty()) {
        QToolTip::hideText();
        QPlainTextEdit::mouseMoveEvent(e);
        return;
    }

    NspNameSpace::instance().loadResources();
    const NspNameSpace::Entry *entry = NspNameSpace::instance().findNode(label);
    if (!entry) {
        QToolTip::showText(e->globalPosition().toPoint(), label, this);
        QPlainTextEdit::mouseMoveEvent(e);
        return;
    }

    QString tip = QString("(%1) %2").arg(entry->type, entry->name);
    if (!entry->desc.isEmpty()) tip += "\n" + entry->desc;
    if (!entry->params.isEmpty()) tip += "\nParameters: " + entry->params;
    if (!entry->returns.isEmpty()) tip += "\nReturns: " + entry->returns;
    if (entry->type == "table") {
        QList<NspNameSpace::Entry> children = NspNameSpace::instance().getList(label);
        if (!children.isEmpty()) {
            tip += "\nMembers:";
            for (const NspNameSpace::Entry &c : children)
                tip += QString("\n    (%1) %2").arg(c.type, c.name);
        }
    }
    QToolTip::showText(e->globalPosition().toPoint(), tip, this);
    QPlainTextEdit::mouseMoveEvent(e);
}

// Extract the identifier at cursorPos. If readPastCursor is true, includes
// characters after the cursor (for tooltip display); otherwise stops at cursor
// (for autocomplete prefix extraction). Handles dotted names like "io.print".
QString CodeEditor::getLabel(int cursorPos, bool readPastCursor) const
{
    QTextDocument *doc = document();
    QTextCursor cursor(doc);
    cursor.setPosition(cursorPos);
    int line = cursor.blockNumber();
    QTextBlock block = doc->findBlockByNumber(line);
    QString lineText = block.text();
    int lineStart = block.position();
    int col = cursorPos - lineStart;
    if (col < 0 || col > lineText.length()) return QString();

    // Read backwards from cursor to extract the identifier
    QString sub;
    for (int i = 0; i < col && i < lineText.length(); i++) {
        QChar p = lineText[i];
        if (sub.isEmpty() && (p == '_' || p == '$' || p.isLetter())) {
            sub += p;
        } else if (p != '_' && !p.isLetterOrNumber() && p != '.') {
            sub.clear();
        } else {
            sub += p;
        }
    }

    // Optionally read forward past cursor position
    if (readPastCursor) {
        for (int i = col; i < lineText.length(); i++) {
            QChar p = lineText[i];
            if (sub.isEmpty() && (p == '_' || p == '$' || p.isLetter())) {
                sub += p;
            } else if (!p.isLetterOrNumber()) {
                break;
            } else {
                sub += p;
            }
        }
    }

    // Strip trailing dot (we want "io" not "io.")
    if (sub.endsWith('.'))
        sub.chop(1);

    return sub;
}

void CodeEditor::setCompleter(NspCompleter *completer)
{
    m_completer = completer;
}

void CodeEditor::setHighlighter(NspSyntaxHighlighter *highlighter)
{
    m_highlighter = highlighter;
}

// Update viewport margins to account for line number gutter + source map width
void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, m_sourceMap->sizeHint().width(), 0);
}

// Click in line number area: if in the fold indicator column, toggle fold
void CodeEditor::lineNumberAreaMousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    int foldSize = fontMetrics().height();
    int numberWidth = lineNumberAreaWidth() - foldSize;
    if (event->position().toPoint().x() < numberWidth) return;

    // Find which block was clicked
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid()) {
        if (event->position().toPoint().y() >= top && event->position().toPoint().y() < bottom) {
            if (isFoldable(block)) {
                toggleFold(block.blockNumber());
                return;
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}

// Track mouse hover in the fold indicator column to show expanded arrows
void CodeEditor::lineNumberAreaMouseMoveEvent(QMouseEvent *event)
{
    int foldSize = fontMetrics().height();
    int numberWidth = lineNumberAreaWidth() - foldSize;
    int x = event->position().toPoint().x();

    bool inFoldColumn = (x >= numberWidth && x < numberWidth + foldSize);
    if (inFoldColumn != m_foldHover) {
        m_foldHover = inFoldColumn;
        m_lineNumberArea->update();
    }
}

void CodeEditor::lineNumberAreaLeaveEvent(QEvent *)
{
    if (m_foldHover) {
        m_foldHover = false;
        m_lineNumberArea->update();
    }
}

// scanRanges - scans a line of text and returns ranges [first, second] (inclusive)
// that are inside strings (quoted), chars, or block comments. Used by folding
// logic to skip braces inside these constructs.
static QVector<QPair<int, int>> scanRanges(const QString &text, int prevState)
{
    QVector<QPair<int, int>> ranges;
    int pos = 0;
    int len = text.length();
    int state = prevState;

    while (pos < len) {
        if (state == 0) {
            // Line comment - rest of line is a range
            if (pos + 1 < len && text[pos] == '/' && text[pos + 1] == '/') {
                ranges.append(qMakePair(pos, len - 1));
                return ranges;
            }
            // Block comment start
            if (pos + 1 < len && text[pos] == '/' && text[pos + 1] == '*') {
                int start = pos;
                pos += 2;
                state = 1;
                while (pos < len) {
                    if (pos + 1 < len && text[pos] == '*' && text[pos + 1] == '/') {
                        pos += 2;
                        ranges.append(qMakePair(start, pos - 1));
                        state = 0;
                        break;
                    }
                    pos++;
                }
                if (state == 1) {
                    // Unclosed block comment
                    ranges.append(qMakePair(start, len - 1));
                }
                continue;
            }
            // Double-quoted string
            if (text[pos] == '"') {
                int start = pos;
                pos++;
                while (pos < len) {
                    if (text[pos] == '\\' && pos + 1 < len) { pos += 2; continue; }
                    if (text[pos] == '"') { pos++; ranges.append(qMakePair(start, pos - 1)); break; }
                    pos++;
                }
                if (pos >= len && (ranges.isEmpty() || ranges.last().second != len - 1)) {
                    ranges.append(qMakePair(start, len - 1));
                }
                continue;
            }
            // Single-quoted char
            if (text[pos] == '\'') {
                int start = pos;
                pos++;
                while (pos < len) {
                    if (text[pos] == '\\' && pos + 1 < len) { pos += 2; continue; }
                    if (text[pos] == '\'') { pos++; ranges.append(qMakePair(start, pos - 1)); break; }
                    pos++;
                }
                if (pos >= len && (ranges.isEmpty() || ranges.last().second != len - 1)) {
                    ranges.append(qMakePair(start, len - 1));
                }
                continue;
            }
            // Backtick-quoted string
            if (text[pos] == '`') {
                int start = pos;
                pos++;
                while (pos < len) {
                    if (text[pos] == '\\' && pos + 1 < len) { pos += 2; continue; }
                    if (text[pos] == '`') { pos++; ranges.append(qMakePair(start, pos - 1)); break; }
                    pos++;
                }
                if (pos >= len && (ranges.isEmpty() || ranges.last().second != len - 1)) {
                    ranges.append(qMakePair(start, len - 1));
                }
                continue;
            }
            pos++;
        } else if (state == 1) {
            // Inside a multi-line block comment continuation
            while (pos < len) {
                if (pos + 1 < len && text[pos] == '*' && text[pos + 1] == '/') {
                    pos += 2;
                    state = 0;
                    break;
                }
                pos++;
            }
        }
    }
    return ranges;
}

// Check if a character position falls inside any range
static bool isInRange(int pos, const QVector<QPair<int, int>> &ranges)
{
    for (const auto &r : ranges) {
        if (pos >= r.first && pos <= r.second) return true;
    }
    return false;
}

// Find the closing brace matching the opening brace on startBlock.
// Skips braces inside strings, chars, and block comments using scanRanges
// and multi-line state from previousBlockState.
int CodeEditor::findMatchingBrace(const QTextBlock &startBlock) const
{
    QTextBlock block = startBlock.next();
    int depth = 1;
    while (block.isValid()) {
        int prevState = block.previous().userState();
        bool inMultiLine = (prevState == 1 || prevState == 2 || prevState == 3 ||
                            prevState == 5 || prevState == 6 || prevState == 7 ||
                            prevState == 8 || prevState == 9);
        if (!inMultiLine) {
            QString text = block.text();
            auto ranges = scanRanges(text, 0);
            for (int i = 0; i < text.length(); i++) {
                if (isInRange(i, ranges)) continue;
                if (text[i] == '{') depth++;
                else if (text[i] == '}') {
                    depth--;
                    if (depth == 0) return block.blockNumber();
                }
            }
        }
        block = block.next();
    }
    return -1;
}

// Determine if a block's content is foldable. Checks:
// - Inside multi-line string/char/comment → not foldable
// - Multi-line block comment start → foldable
// - Net open braces > 0 with matching close on a different line → foldable
// - Keyword + `{` on next line → foldable
bool CodeEditor::isFoldable(const QTextBlock &block) const
{
    if (!m_highlighter) return false;
    if (!block.isValid()) return false;

    // Block is inside a multi-line string/char/comment → not foldable
    int prevState = block.previous().userState();
    if (prevState == 1 || prevState == 2 || prevState == 3 || prevState == 5 || prevState == 6 || prevState == 7 || prevState == 8 || prevState == 9)
        return false;

    // Already folded → still considered foldable (so arrow is shown)
    QTextBlock next = block.next();
    if (next.isValid() && !next.isVisible()) return true;

    QString text = block.text();

    // Skip inline strings and chars to find real brace/comment starts
    bool inString = false;
    bool inChar = false;
    bool inBacktick = false;
    for (int i = 0; i < text.length(); i++) {
        if (inString) {
            if (text[i] == '\\' && i + 1 < text.length()) { i++; continue; }
            if (text[i] == '"') inString = false;
            continue;
        }
        if (inChar) {
            if (text[i] == '\\' && i + 1 < text.length()) { i++; continue; }
            if (text[i] == '\'') inChar = false;
            continue;
        }
        if (inBacktick) {
            if (text[i] == '\\' && i + 1 < text.length()) { i++; continue; }
            if (text[i] == '`') inBacktick = false;
            continue;
        }
        if (text[i] == '"') { inString = true; continue; }
        if (text[i] == '\'') { inChar = true; continue; }
        if (text[i] == '`') { inBacktick = true; continue; }
        if (i + 1 < text.length() && text[i] == '/' && text[i + 1] == '/') break;
        if (i + 1 < text.length() && text[i] == '/' && text[i + 1] == '*') {
            int closePos = text.indexOf("*/", i + 2);
            if (closePos < 0) return true; // Unclosed block comment → foldable
            i = closePos + 1;
            continue;
        }
    }

    auto ranges = scanRanges(text, 0);

    // Check for net open braces that close on another line
    int firstOpenPos = -1;
    int depth = 0;
    for (int i = 0; i < text.length(); i++) {
        if (isInRange(i, ranges)) continue;
        if (text[i] == '{') {
            depth++;
            if (firstOpenPos < 0) firstOpenPos = i;
        } else if (text[i] == '}') {
            depth--;
            if (depth == 0) firstOpenPos = -1;
        }
    }

    if (firstOpenPos >= 0) {
        int endLine = findMatchingBrace(block);
        if (endLine > block.blockNumber()) return true;
    }

    // Check for fold keywords with { on the next line
    static const QStringList keywords = {"else", "for", "while", "do", "try", "catch", "finally", "function", "switch"};
    QString trimmed = text.trimmed();
    for (const QString &kw : keywords) {
        if (trimmed == kw || trimmed.startsWith(kw + " ") || trimmed.startsWith(kw + "(") || trimmed.startsWith(kw + "{")) {
            QTextBlock nb = block.next();
            while (nb.isValid() && nb.text().trimmed().isEmpty()) nb = nb.next();
            if (nb.isValid()) {
                auto nbRanges = scanRanges(nb.text(), 0);
                for (int i = 0; i < nb.text().length(); i++) {
                    if (isInRange(i, nbRanges)) continue;
                    if (nb.text()[i] == '{') return true;
                }
            }
        }
    }

    return false;
}

// Toggle fold/collapse on a block. Handles both brace blocks and block comments.
// Uses QTextBlock::setVisible() and a beginEditBlock/endEditBlock trick to
// force layout recalculation.
void CodeEditor::toggleFold(int blockNumber)
{
    if (m_foldedBlocks.contains(blockNumber)) {
        // Unfold: show all hidden blocks until the matching close
        m_foldedBlocks.remove(blockNumber);
        QTextBlock block = document()->findBlockByNumber(blockNumber);
        block = block.next();
        while (block.isValid() && !block.isVisible()) {
            block.setVisible(true);
            block = block.next();
        }
    } else {
        // Determine what to fold
        QTextBlock startBlock = document()->findBlockByNumber(blockNumber);
        QString text = startBlock.text();

        // Detect block comment
        bool inStr = false;
        bool inCh = false;
        bool isComment = false;
        for (int i = 0; i < text.length(); i++) {
            if (inStr) {
                if (text[i] == '\\' && i + 1 < text.length()) { i++; continue; }
                if (text[i] == '"') inStr = false;
                continue;
            }
            if (inCh) {
                if (text[i] == '\\' && i + 1 < text.length()) { i++; continue; }
                if (text[i] == '\'') inCh = false;
                continue;
            }
            if (text[i] == '"') { inStr = true; continue; }
            if (text[i] == '\'') { inCh = true; continue; }
            if (i + 1 < text.length() && text[i] == '/' && text[i + 1] == '/') break;
            if (i + 1 < text.length() && text[i] == '/' && text[i + 1] == '*') {
                int closePos = text.indexOf("*/", i + 2);
                if (closePos < 0) { isComment = true; }
                else { i = closePos + 1; }
                break;
            }
        }

        if (isComment) {
            // Fold a block comment: hide lines until */ is found
            QTextBlock block = startBlock.next();
            while (block.isValid()) {
                if (block.text().contains("*/")) {
                    m_foldedBlocks.insert(blockNumber);
                    QTextBlock b = document()->findBlockByNumber(blockNumber + 1);
                    while (b.isValid() && b.blockNumber() <= block.blockNumber()) {
                        b.setVisible(false);
                        b = b.next();
                    }
                    goto done;
                }
                block = block.next();
            }
            return;
        }

        // Fold a brace block
        auto ranges = scanRanges(text, 0);

        int bracePos = -1;
        int depth = 0;
        for (int i = 0; i < text.length(); i++) {
            if (isInRange(i, ranges)) continue;
            if (text[i] == '{') {
                depth++;
                bracePos = i;
            } else if (text[i] == '}') {
                depth--;
                if (depth == 0) bracePos = -1;
            }
        }

        // If { is on the next line (e.g. after keyword), look there
        if (bracePos < 0) {
            QTextBlock nextBlock = startBlock.next();
            while (nextBlock.isValid() && nextBlock.text().trimmed().isEmpty())
                nextBlock = nextBlock.next();
            if (nextBlock.isValid()) {
                auto nbRanges = scanRanges(nextBlock.text(), 0);
                for (int i = 0; i < nextBlock.text().length(); i++) {
                    if (isInRange(i, nbRanges)) continue;
                    if (nextBlock.text()[i] == '{') {
                        bracePos = i;
                        break;
                    }
                }
            }
            if (bracePos < 0) return;
        }

        int endLine = findMatchingBrace(startBlock);
        if (endLine < 0) return;
        if (endLine <= blockNumber) return;

        m_foldedBlocks.insert(blockNumber);
        QTextBlock block = document()->findBlockByNumber(blockNumber + 1);
        while (block.isValid() && block.blockNumber() < endLine) {
            block.setVisible(false);
            block = block.next();
        }
    }

done:
    // Force layout recalculation after visibility changes
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    cursor.insertText("");
    cursor.endEditBlock();

    updateLineNumberAreaWidth(0);
    update();
    viewport()->update();
    m_sourceMap->update();
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    m_sourceMap->update();
}

// --- SourceMap ---

SourceMap::SourceMap(CodeEditor *editor, QWidget *parent)
    : QWidget(parent)
    , m_editor(editor)
    , m_dragging(false)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

QSize SourceMap::sizeHint() const
{
    return QSize(80, 0);
}

// Total document height in scroll pixels
int SourceMap::documentHeight() const
{
    return m_editor->verticalScrollBar()->maximum() + m_editor->viewport()->height();
}

// Y pixel in map coordinates corresponding to the first visible line
int SourceMap::visibleFrom() const
{
    int totalLines = m_editor->document()->blockCount();
    if (totalLines <= 1) return 0;
    int topLine = m_editor->firstVisibleBlock().blockNumber();
    return qRound(1.0 * topLine / totalLines * height());
}

// Y pixel in map coordinates corresponding to the last visible line
int SourceMap::visibleTo() const
{
    int totalLines = m_editor->document()->blockCount();
    if (totalLines <= 1) return height();
    QTextBlock block = m_editor->firstVisibleBlock();
    if (!block.isValid()) return height();
    int viewHeight = m_editor->viewport()->height();
    int y = 0;
    while (block.isValid()) {
        y += m_editor->blockBoundingRect(block).height();
        if (y > viewHeight) break;
        block = block.next();
    }
    int lastVisibleLine = block.isValid() ? block.blockNumber() : totalLines;
    return qRound(1.0 * lastVisibleLine / totalLines * height());
}

// Paint the source map minimap. Each non-empty visible line is drawn as a
// colored horizontal line whose width reflects character count. Syntax colors
// are extracted per-character from QTextLayout::FormatRange and blended with
// the editor/map backgrounds to approximate perceived color at minimap scale.
// Leading whitespace is left as background; gaps between syntax tokens are
// forward-filled with the preceding syntax color for visual continuity.
void SourceMap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    bool isDark = m_editor->isDark();

    QPainter painter(this);
    painter.fillRect(rect(), isDark ? QColor(30, 30, 30) : QColor(240, 240, 240));

    QTextDocument *doc = m_editor->document();
    int totalLines = doc->blockCount();
    if (totalLines <= 0) return;

    QColor defaultTextColor = m_editor->palette().color(QPalette::Text);
    QColor mapBg = isDark ? QColor(30, 30, 30) : QColor(240, 240, 240);
    QColor editorBg = m_editor->palette().color(QPalette::Base);
    QColor plainColor = isDark ? QColor(80, 80, 80) : QColor(200, 200, 200);
    int w = width() - 4;

    // Blend a syntax color with the editor background then with the map background
    // to simulate the perceived color when the syntax color is rendered on the
    // editor background and then viewed at reduced scale on the map background.
    auto blendToMap = [&](const QColor &fg) -> QColor {
        QColor base = (fg == plainColor) ? QColor() : fg;
        if (!base.isValid()) return plainColor;
        int r = (base.red() * 0.55 + editorBg.red() * 0.45);
        int g = (base.green() * 0.55 + editorBg.green() * 0.45);
        int b = (base.blue() * 0.55 + editorBg.blue() * 0.45);
        return QColor(
            (r * 0.6 + mapBg.red() * 0.4),
            (g * 0.6 + mapBg.green() * 0.4),
            (b * 0.6 + mapBg.blue() * 0.4));
    };

    for (int i = 0; i < totalLines; i++) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (!block.isValid() || !block.isVisible()) continue;
        QString text = block.text();
        if (text.isEmpty()) continue;

        double yTop = (1.0 * i / totalLines) * height();
        int y = qRound(yTop);

        int lineLen = qMin(text.length(), w);

        // Build per-character color array from syntax highlight format ranges
        QTextLayout *layout = block.layout();
        QList<QTextLayout::FormatRange> formats = layout->formats();

        QVector<QColor> charColors(lineLen, plainColor);
        for (const QTextLayout::FormatRange &fr : formats) {
            QColor c = fr.format.foreground().color();
            if (!c.isValid() || c == defaultTextColor) continue;
            int start = qBound(0, fr.start, lineLen - 1);
            int end = qBound(0, fr.start + fr.length, lineLen);
            for (int j = start; j < end; j++)
                charColors[j] = c;
        }

        // Find the first non-whitespace position (leading whitespace = background)
        int contentStart = 0;
        for (int j = 0; j < lineLen; j++) {
            if (!text[j].isSpace()) { contentStart = j; break; }
        }

        // Forward-fill: any uncolored position after the first syntax token
        // inherits the most recent syntax color (fills gaps between tokens)
        QColor lastColor = plainColor;
        for (int j = contentStart; j < lineLen; j++) {
            if (charColors[j] != plainColor)
                lastColor = charColors[j];
            else if (lastColor != plainColor)
                charColors[j] = lastColor;
        }

        // Mark remaining uncolored positions as invalid (won't be drawn)
        for (int j = contentStart; j < lineLen; j++) {
            if (charColors[j] == plainColor)
                charColors[j] = QColor();
        }

        // Draw colored runs
        int x = contentStart;
        while (x < lineLen) {
            if (!charColors[x].isValid()) { x++; continue; }
            QColor runColor = charColors[x];
            int runStart = x;
            while (x < lineLen && charColors[x] == runColor)
                x++;
            painter.setPen(blendToMap(runColor));
            painter.drawLine(2 + runStart, y, 2 + x, y);
        }
    }

    // Draw viewport indicator rectangle
    int from = visibleFrom();
    int to = visibleTo();
    QColor highlightColor = isDark ? QColor(100, 160, 220, 80) : QColor(41, 128, 185, 80);
    QColor borderColor = isDark ? QColor(100, 160, 220, 180) : QColor(41, 128, 185, 180);
    painter.fillRect(QRect(0, from, width(), qMax(to - from, 1)), highlightColor);
    painter.setPen(borderColor);
    painter.drawRect(QRect(0, from, width() - 1, qMax(to - from - 1, 0)));
}

// Click on source map navigates to the corresponding line
void SourceMap::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;
    int y = event->position().toPoint().y();
    int totalLines = m_editor->document()->blockCount();
    if (totalLines <= 1) return;
    int targetLine = qRound(1.0 * y / height() * totalLines);
    QTextCursor cursor(m_editor->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, targetLine);
    m_editor->setTextCursor(cursor);
    m_editor->ensureCursorVisible();
    update();
}

// Drag on source map scrolls the editor viewport
void SourceMap::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) return;
    int y = event->position().toPoint().y();
    int totalLines = m_editor->document()->blockCount();
    if (totalLines <= 1) return;
    int targetLine = qRound(1.0 * y / height() * totalLines);
    targetLine = qBound(0, targetLine, totalLines - 1);
    QTextCursor cursor(m_editor->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, targetLine);
    m_editor->setTextCursor(cursor);
    m_editor->ensureCursorVisible();
    update();
}

void SourceMap::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
}