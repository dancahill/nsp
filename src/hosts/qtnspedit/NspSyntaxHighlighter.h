#ifndef NSPSYNTAXHIGHLIGHTER_H
#define NSPSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QList>
#include <QPair>

// NspSyntaxHighlighter - syntax highlighting for NSP script files.
// Supports two modes:
//   .ns  mode (m_nspMode=false): pure NSP script highlighting
//   .nsp mode (m_nspMode=true):  HTML template with <?nsp ... ?> blocks
//
// Block state tracking uses composite states:
//   0=Normal, 1=InBlockComment, 2=InString, 3=InChar, 8=InBacktick (outside NSP block)
//   4=InNspBlock (inside NSP block, no multi-line construct)
//   5=InBlockComment+nsp, 6=InString+nsp, 7=InChar+nsp, 9=InBacktick+nsp (inside NSP block)
//
// IMPORTANT: Do NOT add setCurrentBlockState(Normal) at the end of
// highlightNsBlock/highlightNspBlock — it would overwrite multi-line states.
class NspSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit NspSyntaxHighlighter(QTextDocument *parent = nullptr);
    void setNspMode(bool nspMode);
    void refreshDarkMode();             // Re-check OS theme and update colors if changed
    QColor stringColor() const;
    QColor commentColor() const;
    QColor numberColor() const;
    static bool isDarkTheme();

    // Format accessors for tooltip/source map/HTML export use
    QTextCharFormat commentFormat() const { return m_commentFormat; }
    QTextCharFormat stringFormat() const { return m_stringFormat; }
    QTextCharFormat keywordFormat() const { return m_keywordFormat; }
    QTextCharFormat reservedFormat() const { return m_reservedFormat; }
    QTextCharFormat numberFormat() const { return m_numberFormat; }
    QTextCharFormat operatorFormat() const { return m_operatorFormat; }
    QTextCharFormat memberFormat() const { return m_memberFormat; }
    QTextCharFormat tagFormat() const { return m_tagFormat; }

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void initFormats();    // Set up color palette based on dark/light mode

    // First-pass character-by-character scanners that track strings/comments
    // and record their ranges so the second-pass rule engine can skip them.
    void highlightNsBlock(const QString &text, int len, int state, int &pos,
        QList<QPair<int, int>> &stringRanges, QList<QPair<int, int>> &commentRanges);
    void highlightNspBlock(const QString &text, int len, int state, int &pos,
        QList<QPair<int, int>> &stringRanges, QList<QPair<int, int>> &commentRanges,
        QList<QPair<int, int>> &nspRanges);

    QList<HighlightRule> m_rules;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_reservedFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_memberFormat;
    QTextCharFormat m_tagFormat;         // For <?nsp and ?> tags in .nsp mode
    bool m_nspMode;
    bool m_darkMode;
};

#endif // NSPSYNTAXHIGHLIGHTER_H