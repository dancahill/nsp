// NspNameSpace.h - Singleton that loads NSP namespace data from XML resources
// and provides lookup by path (findNode) and child listing (getList) for
// autocomplete and tooltip support in the editor.

#ifndef NSPNAMESPACE_H
#define NSPNAMESPACE_H

#include <QString>
#include <QList>
#include <QMap>
#include <QDomDocument>

class NspNameSpace {
public:
    // Entry represents one node in the NSP namespace hierarchy.
    // name: short name (e.g., "sin"), fullName: dotted path (e.g., "math.sin")
    struct Entry {
        QString name;
        QString fullName;
        QString type;      // "function", "table", "string", "boolean", etc.
        QString desc;       // Human-readable description
        QString params;     // Function parameter signature, e.g., "(x, y)"
        QString returns;    // Return type description
    };

    // Returns the singleton instance of NspNameSpace.
    static NspNameSpace &instance();

    // Loads XML data from embedded resources and optional external files.
    // Only loads once; subsequent calls are no-ops.
    void loadResources();

    // Finds a namespace entry by its dotted path (e.g., "math.sin").
    // Strips leading "_GLOBALS." if present. Returns nullptr if not found.
    const Entry *findNode(const QString &path) const;

    // Returns all direct children of the given parent path.
    // If parentPath is empty, returns top-level entries.
    // Always includes universal methods (gettype, length, tostring).
    QList<Entry> getList(const QString &parentPath) const;

private:
    NspNameSpace();
    // Parses an XML file (from Qt resource system or filesystem) and populates m_entries.
    void loadXml(const QString &source, const QString &location);
    // Recursively parses a QDomElement and its children into m_entries.
    void parseElement(const QDomElement &elem, const QString &parentPath);

    // Map from fullName to Entry for O(1) lookup by dotted path.
    QMap<QString, Entry> m_entries;
    // Whether loadResources() has already been called.
    bool m_loaded;
};

#endif // NSPNAMESPACE_H