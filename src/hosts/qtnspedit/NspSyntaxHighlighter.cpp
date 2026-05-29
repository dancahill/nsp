#include "NspSyntaxHighlighter.h"
#include <QApplication>
#include <QPalette>

NspSyntaxHighlighter::NspSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_nspMode(false)
    , m_darkMode(isDarkTheme())
{
    initFormats();
}

// Switch between .ns (pure script) and .nsp (HTML template) modes
void NspSyntaxHighlighter::setNspMode(bool nspMode)
{
    m_nspMode = nspMode;
    rehighlight();
}

// Detect OS theme change and update highlighter colors accordingly
void NspSyntaxHighlighter::refreshDarkMode()
{
    bool dark = isDarkTheme();
    if (dark != m_darkMode) {
        m_darkMode = dark;
        initFormats();
        rehighlight();
    }
}

QColor NspSyntaxHighlighter::stringColor() const
{
    return m_stringFormat.foreground().color();
}

QColor NspSyntaxHighlighter::commentColor() const
{
    return m_commentFormat.foreground().color();
}

QColor NspSyntaxHighlighter::numberColor() const
{
    return m_numberFormat.foreground().color();
}

bool NspSyntaxHighlighter::isDarkTheme()
{
    return QApplication::palette().color(QPalette::Base).value() < 128;
}

// Set up the color palette and keyword/reserved/number/operator/member rules.
// Dark and light mode use different color schemes.
void NspSyntaxHighlighter::initFormats()
{
    m_rules.clear();

    // Dark mode palette (inspired by VS Code dark theme)
    if (m_darkMode) {
        m_commentFormat.setForeground(QColor(0x6A, 0x99, 0x55));
        m_commentFormat.setFontItalic(true);
        m_stringFormat.setForeground(QColor(0x6A, 0xA0, 0xE8));
        m_keywordFormat.setForeground(QColor(0xB0, 0xB0, 0xB0));
        m_keywordFormat.setFontWeight(QFont::Bold);
        m_reservedFormat.setForeground(QColor(0x7B, 0xB8, 0xE8));
        m_numberFormat.setForeground(QColor(0x80, 0xB8, 0xF0));
        m_operatorFormat.setForeground(QColor(0xE0, 0x60, 0x60));
        m_operatorFormat.setFontWeight(QFont::Bold);
        m_memberFormat.setForeground(QColor(0x9D, 0xD0, 0xFA));
        m_tagFormat.setForeground(QColor(0x56, 0x9C, 0xD6));
        m_tagFormat.setFontWeight(QFont::Bold);
    } else {
        // Light mode palette
        m_commentFormat.setForeground(QColor(0x00, 0x80, 0x00));
        m_commentFormat.setFontItalic(true);
        m_stringFormat.setForeground(QColor(0x00, 0x00, 0xFF));
        m_keywordFormat.setForeground(QColor(0x00, 0x00, 0x00));
        m_keywordFormat.setFontWeight(QFont::Bold);
        m_reservedFormat.setForeground(QColor(0x00, 0x00, 0xFF));
        m_numberFormat.setForeground(QColor(0x00, 0x00, 0x80));
        m_operatorFormat.setForeground(QColor(0x80, 0x00, 0x00));
        m_operatorFormat.setFontWeight(QFont::Bold);
        m_memberFormat.setForeground(QColor(0x00, 0x80, 0x80));
        m_tagFormat.setForeground(QColor(0x00, 0x00, 0xFF));
        m_tagFormat.setFontWeight(QFont::Bold);
    }

    HighlightRule rule;

    // NSP keywords (bold grey in dark, bold black in light)
    QStringList keywordPatterns = {
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bforeach\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bdo\\b"),
        QStringLiteral("\\bswitch\\b"), QStringLiteral("\\bcase\\b"),
        QStringLiteral("\\bdefault\\b"), QStringLiteral("\\btry\\b"),
        QStringLiteral("\\bcatch\\b"), QStringLiteral("\\bfinally\\b"),
        QStringLiteral("\\bthrow\\b"), QStringLiteral("\\bfunction\\b"),
        QStringLiteral("\\bclass\\b"), QStringLiteral("\\bnew\\b"),
        QStringLiteral("\\bdelete\\b"), QStringLiteral("\\breturn\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bvar\\b"), QStringLiteral("\\blocal\\b"),
        QStringLiteral("\\bglobal\\b"), QStringLiteral("\\bnamespace\\b"),
        QStringLiteral("\\bin\\b"), QStringLiteral("\\bexit\\b")
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Reserved words: true, false, null, this
    QStringList reservedPatterns = {
        QStringLiteral("\\btrue\\b"), QStringLiteral("\\bfalse\\b"),
        QStringLiteral("\\bnull\\b"), QStringLiteral("\\bthis\\b")
    };
    for (const QString &pattern : reservedPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_reservedFormat;
        m_rules.append(rule);
    }

    // Hex numbers
    rule.pattern = QRegularExpression(QStringLiteral("\\b0[xX][0-9a-fA-F]+\\b"));
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Decimal numbers (integer and float, with optional exponent)
    rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b"));
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Operators and punctuation
    rule.pattern = QRegularExpression(QStringLiteral("[=+\\-*/%&|^!<>?:]"));
    rule.format = m_operatorFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression(QStringLiteral("[(){};.\\[\\]]"));
    rule.format = m_operatorFormat;
    m_rules.append(rule);

    // Dotted member chain (identifier.identifier.identifier...) — colors all parts
    rule.pattern = QRegularExpression(QStringLiteral("[a-zA-Z_][a-zA-Z0-9_]*(\\.[a-zA-Z_][a-zA-Z0-9_]*)+"));
    rule.format = m_memberFormat;
    m_rules.append(rule);
}

// Main highlighting entry point. Dispatches to the appropriate mode handler,
// then applies regex-based rules excluding positions inside strings/comments
// (and, in .nsp mode, excluding positions outside <?nsp ... ?> blocks).
void NspSyntaxHighlighter::highlightBlock(const QString &text)
{
    enum State { Normal = 0, InBlockComment = 1, InString = 2, InChar = 3, InNspBlock = 4, InBacktick = 8 };

    int state = previousBlockState();
    if (state < 0) state = Normal;

    QList<QPair<int, int>> stringRanges;
    QList<QPair<int, int>> commentRanges;
    QList<QPair<int, int>> nspRanges;

    int len = text.length();
    int pos = 0;

    if (m_nspMode) {
        highlightNspBlock(text, len, state, pos, stringRanges, commentRanges, nspRanges);
    } else {
        highlightNsBlock(text, len, state, pos, stringRanges, commentRanges);
    }

    // Second pass: apply keyword/reserved/number/operator/member rules,
    // but only to positions NOT inside strings or comments (and, in .nsp mode,
    // only to positions INSIDE nspRanges).
    auto isInRange = [](int p, const QList<QPair<int, int>> &ranges) -> bool {
        for (const auto &range : ranges) {
            if (p >= range.first && p <= range.second)
                return true;
        }
        return false;
    };

    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int start = match.capturedStart();
            int end = match.capturedEnd();
            bool skip = false;
            for (int p = start; p < end; p++) {
                if (isInRange(p, stringRanges) || isInRange(p, commentRanges)) {
                    skip = true;
                    break;
                }
                if (m_nspMode && !isInRange(p, nspRanges)) {
                    skip = true;
                    break;
                }
            }
            if (!skip)
                setFormat(start, match.capturedLength(), rule.format);
        }
    }
}

// First-pass scanner for .ns mode (pure NSP script).
// Walks the text character-by-character tracking strings (with escapes),
// chars, block comments, and line comments. Records their ranges so
// the rule engine can skip them. Sets block state for multi-line constructs.
void NspSyntaxHighlighter::highlightNsBlock(const QString &text, int len, int state, int &pos,
    QList<QPair<int, int>> &stringRanges, QList<QPair<int, int>> &commentRanges)
{
    enum State { Normal = 0, InBlockComment = 1, InString = 2, InChar = 3, InBacktick = 8 };

    // Handle continuation from previous block
    if (state == InBlockComment) {
        int end = text.indexOf(QStringLiteral("*/"));
        if (end < 0) {
            commentRanges.append(qMakePair(0, len));
            setFormat(0, len, m_commentFormat);
            setCurrentBlockState(InBlockComment);
            pos = len;
            return;
        }
        commentRanges.append(qMakePair(0, end + 1));
        setFormat(0, end + 2, m_commentFormat);
        setCurrentBlockState(Normal);
        pos = end + 2;
    } else if (state == InString) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '"') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                setCurrentBlockState(Normal);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (!closed) {
            stringRanges.append(qMakePair(0, len));
            setFormat(0, len, m_stringFormat);
            setCurrentBlockState(InString);
            pos = len;
        }
    } else if (state == InChar) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '\'') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                setCurrentBlockState(Normal);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (!closed) {
            stringRanges.append(qMakePair(0, len));
            setFormat(0, len, m_stringFormat);
            setCurrentBlockState(InChar);
            pos = len;
        }
    } else if (state == InBacktick) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '`') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                setCurrentBlockState(Normal);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (!closed) {
            stringRanges.append(qMakePair(0, len));
            setFormat(0, len, m_stringFormat);
            setCurrentBlockState(InBacktick);
            pos = len;
        }
    }

    // Scan forward from pos
    while (pos < len) {
        // Line comment //
        if (text[pos] == '/' && pos + 1 < len) {
            if (text[pos + 1] == '/') {
                commentRanges.append(qMakePair(pos, len));
                setFormat(pos, len - pos, m_commentFormat);
                setCurrentBlockState(Normal);
                pos = len;
                continue;
            }
            // Block comment /*
            if (text[pos + 1] == '*') {
                int end = text.indexOf(QStringLiteral("*/"), pos + 2);
                if (end < 0) {
                    commentRanges.append(qMakePair(pos, len));
                    setFormat(pos, len - pos, m_commentFormat);
                    setCurrentBlockState(InBlockComment);
                    pos = len;
                    continue;
                }
                commentRanges.append(qMakePair(pos, end + 1));
                setFormat(pos, end - pos + 2, m_commentFormat);
                setCurrentBlockState(Normal);
                pos = end + 2;
                continue;
            }
        }

        // # line comment
        if (text[pos] == '#') {
            commentRanges.append(qMakePair(pos, len));
            setFormat(pos, len - pos, m_commentFormat);
            setCurrentBlockState(Normal);
            pos = len;
            continue;
        }

        // Double-quoted string
        if (text[pos] == '"') {
            int start = pos;
            pos++;
            bool escape = false;
            bool closed = false;
            while (pos < len) {
                if (escape) { escape = false; pos++; continue; }
                if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                if (text[pos] == '"') {
                    stringRanges.append(qMakePair(start, pos));
                    setFormat(start, pos - start + 1, m_stringFormat);
                    setCurrentBlockState(Normal);
                    pos++;
                    closed = true;
                    break;
                }
                pos++;
            }
            if (!closed) {
                stringRanges.append(qMakePair(start, len - 1));
                setFormat(start, len - start, m_stringFormat);
                setCurrentBlockState(InString);
            }
            continue;
        }

        // Single-quoted char
        if (text[pos] == '\'') {
            int start = pos;
            pos++;
            bool escape = false;
            bool closed = false;
            while (pos < len) {
                if (escape) { escape = false; pos++; continue; }
                if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                if (text[pos] == '\'') {
                    stringRanges.append(qMakePair(start, pos));
                    setFormat(start, pos - start + 1, m_stringFormat);
                    setCurrentBlockState(Normal);
                    pos++;
                    closed = true;
                    break;
                }
                pos++;
            }
            if (!closed) {
                stringRanges.append(qMakePair(start, len - 1));
                setFormat(start, len - start, m_stringFormat);
                setCurrentBlockState(InChar);
            }
            continue;
        }

        // Backtick-quoted string
        if (text[pos] == '`') {
            int start = pos;
            pos++;
            bool escape = false;
            bool closed = false;
            while (pos < len) {
                if (escape) { escape = false; pos++; continue; }
                if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                if (text[pos] == '`') {
                    stringRanges.append(qMakePair(start, pos));
                    setFormat(start, pos - start + 1, m_stringFormat);
                    setCurrentBlockState(Normal);
                    pos++;
                    closed = true;
                    break;
                }
                pos++;
            }
            if (!closed) {
                stringRanges.append(qMakePair(start, len - 1));
                setFormat(start, len - start, m_stringFormat);
                setCurrentBlockState(InBacktick);
            }
            continue;
        }

        pos++;
    }
}

// First-pass scanner for .nsp mode (HTML template with <?nsp ... ?> blocks).
// In addition to string/char/comment tracking, this handles the <?nsp and ?>
// tag delimiters and tracks whether we're inside an NSP block or in plain HTML.
// Composite states 4-7 and 9 ensure multi-line constructs inside NSP blocks revert
// to InNspBlock (4) rather than Normal (0).
void NspSyntaxHighlighter::highlightNspBlock(const QString &text, int len, int state, int &pos,
    QList<QPair<int, int>> &stringRanges, QList<QPair<int, int>> &commentRanges,
    QList<QPair<int, int>> &nspRanges)
{
    // States 0-3, 8: outside NSP block; States 4-7, 9: inside NSP block
    // 0=Normal, 1=InBlockComment, 2=InString, 3=InChar, 8=InBacktick
    // 4=InNspBlock, 5=InBlockComment+nsp, 6=InString+nsp, 7=InChar+nsp, 9=InBacktick+nsp
    const int Normal = 0, InBlockComment = 1, InString = 2, InChar = 3, InNspBlock = 4, InBacktick = 8;

    bool inNsp = (state >= InNspBlock);

    // Handle continuation from previous block
    if (state == InBlockComment || state == 5) {
        int end = text.indexOf(QStringLiteral("*/"));
        if (end < 0) {
            commentRanges.append(qMakePair(0, len - 1));
            setFormat(0, len, m_commentFormat);
            setCurrentBlockState(inNsp ? 5 : InBlockComment);
            pos = len;
            return;
        }
        commentRanges.append(qMakePair(0, end + 1));
        setFormat(0, end + 2, m_commentFormat);
        pos = end + 2;
        if (inNsp) {
            nspRanges.append(qMakePair(0, len - 1));
            setCurrentBlockState(InNspBlock);
        } else {
            setCurrentBlockState(Normal);
        }
    } else if (state == InString || state == 6) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '"') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (inNsp) {
            nspRanges.append(qMakePair(0, len - 1));
            if (closed) {
                setCurrentBlockState(InNspBlock);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(6);
                pos = len;
                return;
            }
        } else {
            if (closed) {
                setCurrentBlockState(Normal);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(InString);
                pos = len;
                return;
            }
        }
    } else if (state == InChar || state == 7) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '\'') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (inNsp) {
            nspRanges.append(qMakePair(0, len - 1));
            if (closed) {
                setCurrentBlockState(InNspBlock);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(7);
                pos = len;
                return;
            }
        } else {
            if (closed) {
                setCurrentBlockState(Normal);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(InChar);
                pos = len;
                return;
            }
        }
    } else if (state == InBacktick || state == 9) {
        int i = 0;
        bool escape = false;
        bool closed = false;
        while (i < len) {
            if (escape) { escape = false; i++; continue; }
            if (text[i] == '\\' && i + 1 < len) { escape = true; i++; continue; }
            if (text[i] == '`') {
                stringRanges.append(qMakePair(0, i));
                setFormat(0, i + 1, m_stringFormat);
                pos = i + 1;
                closed = true;
                break;
            }
            i++;
        }
        if (inNsp) {
            nspRanges.append(qMakePair(0, len - 1));
            if (closed) {
                setCurrentBlockState(InNspBlock);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(9);
                pos = len;
                return;
            }
        } else {
            if (closed) {
                setCurrentBlockState(Normal);
            } else {
                stringRanges.append(qMakePair(0, len - 1));
                setFormat(0, len, m_stringFormat);
                setCurrentBlockState(InBacktick);
                pos = len;
                return;
            }
        }
    }

    // Handle continuation of an NSP block from a previous line
    if (state == InNspBlock) {
        int endTag = text.indexOf(QStringLiteral("?>"));
        if (endTag < 0) {
            nspRanges.append(qMakePair(0, len - 1));
            pos = 0;
            setCurrentBlockState(InNspBlock);
        } else {
            if (endTag > 0)
                nspRanges.append(qMakePair(0, endTag - 1));
            setFormat(endTag, 2, m_tagFormat);
            pos = endTag + 2;
            inNsp = false;
            setCurrentBlockState(Normal);
        }
    }

    // Scan forward
    while (pos < len) {
        // <?nsp opening tag
        if (text.mid(pos, 5) == "<?nsp") {
            int tagStart = pos;
            int openEnd = pos + 5;
            int endTag = text.indexOf(QStringLiteral("?>"), openEnd);
            if (endTag < 0) {
                // Unclosed NSP block — rest of line is NSP code
                nspRanges.append(qMakePair(openEnd, len - 1));
                setFormat(tagStart, 5, m_tagFormat);
                setCurrentBlockState(InNspBlock);
                inNsp = true;
                pos = openEnd;
                continue;
            }
            nspRanges.append(qMakePair(openEnd, endTag - 1));
            setFormat(tagStart, 5, m_tagFormat);
            setFormat(endTag, 2, m_tagFormat);
            pos = endTag + 2;
            if (text.indexOf(QStringLiteral("?>"), pos) >= 0 || text.indexOf(QStringLiteral("<?nsp"), pos) >= 0) {
                // more tags on this line, don't set state yet
            } else if (text.indexOf(QStringLiteral("<?nsp"), pos) < 0) {
                bool moreNsp = false;
                for (const auto &range : nspRanges) {
                    if (pos >= range.first && pos <= range.second) { moreNsp = true; break; }
                }
                setCurrentBlockState(moreNsp ? InNspBlock : Normal);
            }
            inNsp = false;
            continue;
        }

        // ?> closing tag (outside NSP context — stray)
        if (!inNsp && text.mid(pos, 2) == "?>") {
            setFormat(pos, 2, m_tagFormat);
            pos += 2;
            inNsp = false;
            continue;
        }

        bool posInNsp = inNsp;
        if (!posInNsp) {
            for (const auto &range : nspRanges) {
                if (pos >= range.first && pos <= range.second) { posInNsp = true; break; }
            }
        }

        // Only apply NSP syntax highlighting inside <?nsp ... ?> blocks
        if (posInNsp) {
            // Line comment //
            if (text[pos] == '/' && pos + 1 < len) {
                if (text[pos + 1] == '/') {
                    commentRanges.append(qMakePair(pos, len - 1));
                    setFormat(pos, len - pos, m_commentFormat);
                    setCurrentBlockState(inNsp ? InNspBlock : Normal);
                    pos = len;
                    continue;
                }
                // Block comment /*
                if (text[pos + 1] == '*') {
                    int end = text.indexOf(QStringLiteral("*/"), pos + 2);
                    if (end < 0) {
                        commentRanges.append(qMakePair(pos, len - 1));
                        setFormat(pos, len - pos, m_commentFormat);
                        setCurrentBlockState(inNsp ? 5 : InBlockComment);
                        pos = len;
                        continue;
                    }
                    commentRanges.append(qMakePair(pos, end + 1));
                    setFormat(pos, end - pos + 2, m_commentFormat);
                    pos = end + 2;
                    continue;
                }
            }

            // # line comment
            if (text[pos] == '#') {
                commentRanges.append(qMakePair(pos, len - 1));
                setFormat(pos, len - pos, m_commentFormat);
                setCurrentBlockState(inNsp ? InNspBlock : Normal);
                pos = len;
                continue;
            }

            // Double-quoted string inside NSP block
            if (text[pos] == '"') {
                int start = pos;
                pos++;
                bool escape = false;
                bool closed = false;
                while (pos < len) {
                    if (escape) { escape = false; pos++; continue; }
                    if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                    if (text[pos] == '"') {
                        stringRanges.append(qMakePair(start, pos));
                        setFormat(start, pos - start + 1, m_stringFormat);
                        pos++;
                        closed = true;
                        break;
                    }
                    pos++;
                }
                if (!closed) {
                    stringRanges.append(qMakePair(start, len - 1));
                    setFormat(start, len - start, m_stringFormat);
                    setCurrentBlockState(inNsp ? 6 : InString);
                    pos = len;
                    continue;
                }
                continue;
            }

            // Single-quoted char inside NSP block
            if (text[pos] == '\'') {
                int start = pos;
                pos++;
                bool escape = false;
                bool closed = false;
                while (pos < len) {
                    if (escape) { escape = false; pos++; continue; }
                    if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                    if (text[pos] == '\'') {
                        stringRanges.append(qMakePair(start, pos));
                        setFormat(start, pos - start + 1, m_stringFormat);
                        pos++;
                        closed = true;
                        break;
                    }
                    pos++;
                }
                if (!closed) {
                    stringRanges.append(qMakePair(start, len - 1));
                    setFormat(start, len - start, m_stringFormat);
                    setCurrentBlockState(inNsp ? 7 : InChar);
                    pos = len;
                    continue;
                }
                continue;
            }

            // Backtick-quoted string inside NSP block
            if (text[pos] == '`') {
                int start = pos;
                pos++;
                bool escape = false;
                bool closed = false;
                while (pos < len) {
                    if (escape) { escape = false; pos++; continue; }
                    if (text[pos] == '\\' && pos + 1 < len) { escape = true; pos++; continue; }
                    if (text[pos] == '`') {
                        stringRanges.append(qMakePair(start, pos));
                        setFormat(start, pos - start + 1, m_stringFormat);
                        pos++;
                        closed = true;
                        break;
                    }
                    pos++;
                }
                if (!closed) {
                    stringRanges.append(qMakePair(start, len - 1));
                    setFormat(start, len - start, m_stringFormat);
                    setCurrentBlockState(inNsp ? 9 : InBacktick);
                    pos = len;
                    continue;
                }
                continue;
            }
        }

        pos++;
    }

    // Final state: set InNspBlock if still inside an NSP block at end of line
    if (inNsp)
        setCurrentBlockState(InNspBlock);
    else
        setCurrentBlockState(Normal);
}