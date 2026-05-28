// NspFormatter.cpp - Implementation of NSP code formatting.
// format() normalizes indentation (tabs), spacing around operators, commas,
// semicolons, braces, etc. for .ns files. formatNsp() splits .nsp templates
// into HTML and NSP blocks, formatting only the NSP code portions.

#include "NspFormatter.h"

#include <QStringList>
#include <QSet>

// isIdentChar: Returns true if the character is a valid NSP identifier character
// (letter, digit, underscore, or dollar sign).
static bool isIdentChar(QChar c)
{
    return c.isLetterOrNumber() || c == '_' || c == '$';
}

// isSpaceBeforeParenKeyword: Returns true if the string so far ends with a
// keyword that should have a space before an opening parenthesis (e.g.,
// "if (", "for (", "while (" — but not "func(" for function calls).
static bool isSpaceBeforeParenKeyword(const QString &result)
{
    static const QStringList keywords = {
        "if", "else", "for", "foreach", "while", "do",
        "switch", "case", "catch", "finally", "return",
        "throw", "new", "delete", "typeof", "sizeof"
    };
    for (const QString &kw : keywords) {
        if (result.endsWith(kw)) {
            int kwLen = kw.length();
            // Ensure the keyword is not part of a longer identifier
            if (result.length() == kwLen || !isIdentChar(result[result.length() - kwLen - 1]))
                return true;
        }
    }
    return false;
}

// normalizeLine: Processes a single line of code, normalizing whitespace:
// - Collapses runs of spaces/tabs into single spaces
// - Adds spaces after commas and semicolons
// - Adds spaces around binary operators: +, -, *, /, %, =, ==, !=, <, >, <=, >=, &&, ||, ^, ?
// - No space after ! (unary negation)
// - No space before ( after function names; space before ( after keywords
// - Handles ++ and -- as compound operators
// - Detects unary +/- and omits the space
// - Colon gets space before it unless after case/default/public/private/protected
// - Preserves strings, escapes, line comments (#, //), inline block comments
static QString normalizeLine(const QString &line)
{
    QString result;
    bool inString = false;
    bool escape = false;
    QChar stringChar;

    for (int i = 0; i < line.size(); i++) {
        QChar ch = line[i];

        // Handle escape sequences inside strings
        if (escape) {
            result += ch;
            escape = false;
            continue;
        }

        // Inside a string: copy characters verbatim until the closing quote
        if (inString) {
            result += ch;
            if (ch == '\\' && i + 1 < line.size())
                escape = true;
            else if (ch == stringChar)
                inString = false;
            continue;
        }

        // Start of a quoted string
        if (ch == '"' || ch == '\'') {
            inString = true;
            stringChar = ch;
            result += ch;
            continue;
        }

        // # line comment: preserve rest of line as-is
        if (ch == '#') {
            result += line.mid(i);
            break;
        }

        // // line comment: preserve rest of line as-is
        if (ch == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            result += line.mid(i);
            break;
        }

        // /* inline block comment: preserve as-is, find closing */
        if (ch == '/' && i + 1 < line.size() && line[i + 1] == '*') {
            int endComment = line.indexOf("*/", i + 2);
            if (endComment >= 0) {
                result += line.mid(i, endComment - i + 2);
                i = endComment + 1;
            } else {
                result += line.mid(i);
                break;
            }
            continue;
        }

        // Collapse runs of whitespace into single space
        if (ch == ' ' || ch == '\t') {
            if (!result.isEmpty() && result.back() != ' ')
                result += ' ';
            continue;
        }

        // Comma and semicolon: add space after if not already present
        if (ch == ',' || ch == ';') {
            result += ch;
            if (i + 1 < line.size() && line[i + 1] != ' ')
                result += ' ';
            continue;
        }

        // Opening brackets: ( [ {
        if (ch == '(' || ch == '[' || ch == '{') {
            if (ch == '(') {
                // Space before ( only after keywords like if, for, while
                if (isSpaceBeforeParenKeyword(result)) {
                    if (!result.isEmpty() && result.back() != ' ')
                        result += ' ';
                }
            } else if (ch == '[' || ch == '{') {
                // Space before [ or { unless after certain punctuation or identifiers
                if (!result.isEmpty() && result.back() != ' '
                    && result.back() != '(' && result.back() != '[' && result.back() != '{'
                    && result.back() != ']' && result.back() != ')'
                    && result.back() != '}' && result.back() != '='
                    && result.back() != ',' && result.back() != '!'
                    && result.back() != '+' && !isIdentChar(result.back()))
                    result += ' ';
            }
            result += ch;
            continue;
        }

        // Closing brackets: ) ] — add space after if followed by identifier char
        if (ch == ')' || ch == ']') {
            result += ch;
            if (i + 1 < line.size() && isIdentChar(line[i + 1]))
                result += ' ';
            continue;
        }

        // } is handled separately: no space after
        if (ch == '}') {
            result += ch;
            continue;
        }

        // . member access: no spaces
        if (ch == '.') {
            result += ch;
            continue;
        }

        // ++ increment operator: treat as compound, no space inside
        if (ch == '+' && i + 1 < line.size() && line[i + 1] == '+') {
            result += "++";
            i++;
            // Space after ++ unless followed by ), ], ;, ,, or }
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ']' && line[i + 1] != ';'
                && line[i + 1] != ',' && line[i + 1] != '}')
                result += ' ';
            continue;
        }

        // -- decrement operator: treat as compound, no space inside
        if (ch == '-' && i + 1 < line.size() && line[i + 1] == '-') {
            result += "--";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ']' && line[i + 1] != ';'
                && line[i + 1] != ',' && line[i + 1] != '}')
                result += ' ';
            continue;
        }

        // Detect unary +/- (after operators, brackets, commas, or at start of expression)
        bool isUnaryMinus = false;
        if (ch == '-' || ch == '+') {
            if (result.isEmpty() || result.back() == '(' || result.back() == '['
                || result.back() == ',' || result.back() == '!'
                || result.back() == '=' || result.back() == '<' || result.back() == '>'
                || result.back() == '&' || result.back() == '|'
                || result.back() == '+' || result.back() == '-'
                || result.back() == '*' || result.back() == '/'
                || result.back() == '%'
                || result.back() == '{' || result.back() == '}') {
                isUnaryMinus = true;
            }
        }

        // Unary +/-: no spaces around it (e.g., x = -1)
        if (isUnaryMinus) {
            result += ch;
            continue;
        }

        // != operator: space before and after
        if (ch == '!' && i + 1 < line.size() && line[i + 1] == '=') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += "!=";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ']' && line[i + 1] != ';' && line[i + 1] != ',')
                result += ' ';
            continue;
        }

        // == operator: space before and after
        if (ch == '=' && i + 1 < line.size() && line[i + 1] == '=') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += "==";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ']' && line[i + 1] != ';' && line[i + 1] != ',')
                result += ' ';
            continue;
        }

        // <= operator: space before and after
        if (ch == '<' && i + 1 < line.size() && line[i + 1] == '=') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += "<=";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ']')
                result += ' ';
            continue;
        }

        // >= operator: space before and after
        if (ch == '>' && i + 1 < line.size() && line[i + 1] == '=') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += ">=";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ')'
                && line[i + 1] != ';' && line[i + 1] != ',')
                result += ' ';
            continue;
        }

        // && operator: space before and after
        if (ch == '&' && i + 1 < line.size() && line[i + 1] == '&') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += "&&";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ') result += ' ';
            continue;
        }

        // || operator: space before and after
        if (ch == '|' && i + 1 < line.size() && line[i + 1] == '|') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += "||";
            i++;
            if (i + 1 < line.size() && line[i + 1] != ' ') result += ' ';
            continue;
        }

        // Single-character binary operators: = < > + - * / % ^
        // Space before and after, unless the next char is = (handled by
        // the compound operators above).
        if (ch == '=' || ch == '<' || ch == '>' || ch == '+'
            || ch == '-' || ch == '*' || ch == '/' || ch == '%'
            || ch == '^') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += ch;
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != '=')
                result += ' ';
            continue;
        }

        // ! unary negation: no space after
        if (ch == '!') {
            result += ch;
            continue;
        }

        // ? ternary operator: space before and after
        if (ch == '?') {
            if (!result.isEmpty() && result.back() != ' ') result += ' ';
            result += '?';
            if (i + 1 < line.size() && line[i + 1] != ' ')
                result += ' ';
            continue;
        }

        // : colon: space before unless after case/default/public/private/protected
        if (ch == ':') {
            bool noSpaceBefore = false;
            static const QStringList colonKeywords = {
                "case", "default", "public", "private", "protected"
            };
            for (const QString &kw : colonKeywords) {
                if (result.endsWith(kw) && (result.length() == kw.length()
                    || !result[result.length() - kw.length() - 1].isLetterOrNumber()))
                    noSpaceBefore = true;
            }
            // Also check the original line for case/default at the start
            QString lowerOrig = line.toLower();
            if (lowerOrig.startsWith("case") && (lowerOrig.length() == 4 || !lowerOrig[4].isLetterOrNumber()))
                noSpaceBefore = true;
            if (lowerOrig.startsWith("default") && (lowerOrig.length() == 7 || !lowerOrig[7].isLetterOrNumber()))
                noSpaceBefore = true;
            if (!result.isEmpty() && result.back() != ' ' && result.back() != ':' && !noSpaceBefore)
                result += ' ';
            result += ':';
            // Space after colon unless followed by ), ], ;, ,, }, or another colon
            if (i + 1 < line.size() && line[i + 1] != ' ' && line[i + 1] != ':'
                && line[i + 1] != ')' && line[i + 1] != ']' && line[i + 1] != ';'
                && line[i + 1] != ',' && line[i + 1] != '}')
                result += ' ';
            continue;
        }

        // Default: any other character is copied as-is
        result += ch;
    }

    // Trim trailing spaces
    while (result.endsWith(' '))
        result.chop(1);

    return result;
}

// countBraces: Counts the net brace depth change in a line of code.
// Skips strings, block comments, and line comments to accurately count
// only braces in actual code. Used by format() to determine indentation.
static int countBraces(const QString &line, bool inString, QChar stringChar)
{
    int count = 0;
    bool escape = false;
    bool localInString = inString;
    QChar localStringChar = stringChar;

    for (int i = 0; i < line.size(); i++) {
        QChar ch = line[i];
        if (escape) { escape = false; continue; }
        // Inside a string: skip all characters until the closing quote
        if (localInString) {
            if (ch == '\\' && i + 1 < line.size()) { escape = true; continue; }
            if (ch == localStringChar) localInString = false;
            continue;
        }
        // Start of a string literal
        if (ch == '"' || ch == '\'') {
            localInString = true;
            localStringChar = ch;
            continue;
        }
        // # line comment: stop counting
        if (ch == '#') break;
        // // and /* comments: skip over them
        if (ch == '/' && i + 1 < line.size()) {
            if (line[i + 1] == '/') break;
            if (line[i + 1] == '*') {
                int end = line.indexOf("*/", i + 2);
                if (end >= 0) { i = end + 1; continue; }
                break;
            }
        }
        if (ch == '{') count++;
        else if (ch == '}') count--;
    }
    return count;
}

// lineOpensString: Checks whether a line opens an unterminated string.
// Updates inString/stringChar to track state across multiline strings.
// Returns true if the string continues past the end of the line.
static bool lineOpensString(const QString &line, bool &inString, QChar &stringChar)
{
    bool escape = false;
    for (int i = 0; i < line.size(); i++) {
        QChar ch = line[i];
        if (inString) {
            if (escape) { escape = false; continue; }
            if (ch == '\\' && i + 1 < line.size()) { escape = true; continue; }
            if (ch == stringChar) inString = false;
            continue;
        }
        // # line comment: stop scanning
        if (ch == '#') break;
        // // line comment: stop scanning
        if (ch == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
        // /* block comment: skip to closing */
        if (ch == '/' && i + 1 < line.size() && line[i + 1] == '*') {
            int end = line.indexOf("*/", i + 2);
            if (end >= 0) { i = end + 1; continue; }
            // Unterminated block comment on this line
            return false;
        }
        if (ch == '"' || ch == '\'') {
            inString = true;
            stringChar = ch;
            continue;
        }
    }
    return inString;
}

// NspFormatter::format
// Main formatting function for .ns files. Normalizes each line with
// normalizeLine(), computes indentation based on brace depth, and handles
// special cases like block comments, multiline strings, case/default
// alignment, and else/closing-brace indentation. Uses tab characters for
// indentation (4-wide tab stops matching CodeEditor).
QString NspFormatter::format(const QString &code)
{
    QStringList input = code.split('\n');
    QStringList output;
    int indent = 0;
    bool inBlockComment = false;
    bool inMultilineString = false;
    QChar mlStringChar;

    for (int i = 0; i < input.size(); i++) {
        QString raw = input[i].trimmed();

        // Inside a multiline string: preserve content as-is, just add indentation
        if (inMultilineString) {
            output << QString(indent, '\t') + raw;
            bool esc = false;
            for (int c = 0; c < raw.size(); c++) {
                if (esc) { esc = false; continue; }
                if (raw[c] == '\\' && c + 1 < raw.size()) { esc = true; continue; }
                if (raw[c] == mlStringChar) { inMultilineString = false; break; }
            }
            continue;
        }

        // Inside a block comment: preserve as-is, end when */ is found
        if (inBlockComment) {
            output << QString(indent, '\t') + raw;
            if (raw.contains("*/"))
                inBlockComment = false;
            continue;
        }

        // Blank lines: output empty (no indentation)
        if (raw.isEmpty()) {
            output << QString();
            continue;
        }

        // Line comments: preserve as-is with current indentation
        if (raw.startsWith("//") || raw.startsWith("#")) {
            output << QString(indent, '\t') + raw;
            continue;
        }

        // Block comment start: preserve as-is, mark if unterminated
        if (raw.startsWith("/*")) {
            output << QString(indent, '\t') + raw;
            if (!raw.contains("*/"))
                inBlockComment = true;
            continue;
        }

        // Normal code line: normalize spacing
        QString trimmed = normalizeLine(raw);

        // Count net brace change for this line
        int braceNet = countBraces(trimmed, false, QChar());

        // Determine if this line should be outdented (closing brace, else, case/default)
        bool startsWithCloseBrace = trimmed.startsWith('}');
        bool startsWithElse = false;
        bool startsWithCaseOrDefault = false;
        QString lower = trimmed.toLower();
        if (lower.startsWith("else") && (lower.length() == 4 || !lower[4].isLetterOrNumber()))
            startsWithElse = true;
        else if (lower.startsWith("} else") || lower.startsWith("}else"))
            startsWithElse = true;
        if (lower.startsWith("case") && (lower.length() == 4 || !lower[4].isLetterOrNumber()))
            startsWithCaseOrDefault = true;
        if (lower.startsWith("default") && (lower.length() == 7 || !lower[7].isLetterOrNumber()))
            startsWithCaseOrDefault = true;

        // Outdent lines that start with }, else, case, or default
        int lineIndent = indent;
        if (startsWithCloseBrace || startsWithElse || startsWithCaseOrDefault)
            lineIndent = qMax(0, indent - 1);

        output << QString(lineIndent, '\t') + trimmed;

        // Adjust indentation for the next line
        int net = braceNet;
        // } else { and similar: the } already reduced indent, so compensate
        if (startsWithCloseBrace || startsWithElse)
            net++;
        // case/default with no braces: indent the body one level deeper
        if (startsWithCaseOrDefault && braceNet == 0)
            net++;

        indent = lineIndent + qMax(0, net);
        if (indent < 0) indent = 0;

        // Check if this line opens an unterminated string (for multiline strings)
        bool sIn = false;
        QChar sChar;
        lineOpensString(trimmed, sIn, sChar);
        if (sIn) {
            inMultilineString = true;
            mlStringChar = sChar;
        }
    }

    // Remove trailing blank lines, then add exactly one trailing newline
    while (!output.isEmpty() && output.last().isEmpty())
        output.removeLast();

    if (!output.isEmpty() && !output.last().isEmpty())
        output << QString();

    return output.join('\n');
}

// NspFormatter::formatNsp
// Formats .nsp template files that mix HTML with <?nsp ... ?> blocks.
// HTML outside the blocks is preserved as-is. NSP code inside the blocks
// is extracted, passed through format(), and put back with the tags.
QString NspFormatter::formatNsp(const QString &code)
{
    QStringList lines = code.split('\n');
    QStringList result;
    QString nspBlock;
    bool inNspBlock = false;

    for (int i = 0; i < lines.size(); i++) {
        QString line = lines[i];

        if (!inNspBlock) {
            // Look for <?nsp tag opening
            int tagPos = line.indexOf("<?nsp");
            if (tagPos >= 0) {
                int endTagPos = line.indexOf("?>", tagPos + 5);
                if (endTagPos >= 0) {
                    // Inline <?nsp ... ?> on a single line: format just the code portion
                    QString before = line.left(tagPos);
                    QString nspCode = line.mid(tagPos + 5, endTagPos - tagPos - 5).trimmed();
                    QString after = line.mid(endTagPos + 2);
                    if (!nspCode.isEmpty())
                        nspCode = " " + format(nspCode).trimmed() + " ";
                    result << before + "<?nsp" + nspCode + "?>" + after;
                } else {
                    // Multiline <?nsp block starts here
                    result << line.left(tagPos) + "<?nsp";
                    nspBlock = line.mid(tagPos + 5);
                    inNspBlock = true;
                }
            } else {
                // Plain HTML line: preserve as-is
                result << line;
            }
        } else {
            // Inside a <?nsp block: look for ?> closing tag
            int endTagPos = line.indexOf("?>");
            if (endTagPos >= 0) {
                // Block ends: format the accumulated NSP code and close
                nspBlock += line.left(endTagPos);
                if (!nspBlock.trimmed().isEmpty()) {
                    QStringList formatted = format(nspBlock).split('\n');
                    for (const QString &fl : formatted)
                        result << fl;
                }
                nspBlock.clear();
                inNspBlock = false;
                result << "?>";
                QString after = line.mid(endTagPos + 2);
                if (!after.trimmed().isEmpty())
                    result << after;
            } else {
                // Continue accumulating NSP code lines
                nspBlock += line + "\n";
            }
        }
    }

    // Handle unterminated NSP block at end of file
    if (inNspBlock && !nspBlock.trimmed().isEmpty()) {
        QStringList formatted = format(nspBlock).split('\n');
        for (const QString &fl : formatted)
            result << fl;
    }

    return result.join('\n');
}