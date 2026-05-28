// NspFormatter.h - Static class providing code formatting for NSP scripts.
// format() handles .ns files (pure NSP code); formatNsp() handles .nsp files
// (HTML templates with <?nsp ... ?> blocks), formatting only the NSP code
// inside the tags while preserving surrounding HTML.

#ifndef NSPFORMATTER_H
#define NSPFORMATTER_H

#include <QString>

class NspFormatter {
public:
    // Formats pure NSP script code: normalizes indentation, spacing, and braces.
    static QString format(const QString &code);

    // Formats .nsp template code: preserves HTML outside <?nsp ... ?> blocks
    // and formats only the NSP code within the tags.
    static QString formatNsp(const QString &code);

private:
    // Deleted constructor: this class is purely static.
    NspFormatter() = delete;
};

#endif // NSPFORMATTER_H