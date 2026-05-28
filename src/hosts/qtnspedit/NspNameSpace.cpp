// NspNameSpace.cpp - Implementation of the NspNameSpace singleton.
// Loads namespace definitions from embedded XML (Qt resources), the executable
// directory, and the current working directory. Provides findNode() for
// lookups by dotted path and getList() for listing children of a namespace.

#include "NspNameSpace.h"

#include <QFile>
#include <QCoreApplication>
#include <QDir>

// Singleton accessor using a function-local static to guarantee initialization order.
NspNameSpace &NspNameSpace::instance()
{
    static NspNameSpace inst;
    return inst;
}

NspNameSpace::NspNameSpace()
    : m_loaded(false)
{
}

// NspNameSpace::loadResources
// Loads XML namespace data from three sources in order:
// 1. Embedded Qt resource (":/resources/NSPNameSpace.xml")
// 2. File beside the executable ("NSPNameSpace.xml" in app dir)
// 3. File in the current working directory
// Only loads once; subsequent calls return immediately.
void NspNameSpace::loadResources()
{
    if (m_loaded) return;
    m_loaded = true;

    // Always load the embedded resource first as the base dataset
    loadXml(":/resources/NSPNameSpace.xml", "embedded");

    // Optionally supplement with external XML files for user extensions
    QString exePath = QCoreApplication::applicationDirPath();
    QString exeDirFile = exePath + "/NSPNameSpace.xml";
    if (QFile::exists(exeDirFile))
        loadXml(exeDirFile, "exedir");

    QString cwdFile = QDir::currentPath() + "/NSPNameSpace.xml";
    if (QFile::exists(cwdFile))
        loadXml(cwdFile, "cwd");
}

// NspNameSpace::loadXml
// Opens and parses an XML file containing <NSPNameSpace> definitions.
// Each top-level element under the root becomes a namespace entry,
// with child elements forming the dotted path hierarchy.
void NspNameSpace::loadXml(const QString &source, const QString &location)
{
    QFile file(source);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomElement root = doc.documentElement();
    if (root.tagName() != "NSPNameSpace")
        return;

    // Parse each top-level element; children are handled recursively
    QDomNode child = root.firstChild();
    while (!child.isNull()) {
        QDomElement elem = child.toElement();
        if (!elem.isNull())
            parseElement(elem, "");
        child = child.nextSibling();
    }
}

// NspNameSpace::parseElement
// Recursively processes a QDomElement into an Entry and inserts it
// into m_entries. The fullName is built by concatenating the parent
// path with the element's tag name using dots (e.g., "math.sin").
// Child elements are parsed with the current fullName as their parent path.
void NspNameSpace::parseElement(const QDomElement &elem, const QString &parentPath)
{
    Entry entry;
    entry.name = elem.tagName();
    entry.fullName = parentPath.isEmpty() ? elem.tagName() : parentPath + "." + elem.tagName();
    entry.type = elem.attribute("type", "");
    entry.desc = elem.attribute("desc", "");
    entry.params = elem.attribute("params", "");
    entry.returns = elem.attribute("returns", "");

    m_entries.insert(entry.fullName, entry);

    // Recurse into child elements to build deeper namespace paths
    QDomNode child = elem.firstChild();
    while (!child.isNull()) {
        QDomElement childElem = child.toElement();
        if (!childElem.isNull())
            parseElement(childElem, entry.fullName);
        child = child.nextSibling();
    }
}

// NspNameSpace::findNode
// Looks up a namespace entry by its dotted path (e.g., "math.sin").
// Strips a leading "_GLOBALS." prefix if present, since the XML
// data may reference entries that way. Returns nullptr if not found.
const NspNameSpace::Entry *NspNameSpace::findNode(const QString &path) const
{
    QString key = path;
    // "_GLOBALS." prefix is used internally by NSP but not in our XML data
    if (key.startsWith("_GLOBALS."))
        key = key.mid(8);
    key = key.trimmed();
    auto it = m_entries.constFind(key);
    if (it != m_entries.constEnd())
        return &it.value();
    return nullptr;
}

// NspNameSpace::getList
// Returns all direct children of the given parentPath. For example,
// passing "math" returns Entry objects for sin, cos, sqrt, etc.
// If parentPath is empty, returns all top-level entries.
// Always appends universal methods (gettype, length, tostring) that
// are available on all objects but may not be in the XML data.
QList<NspNameSpace::Entry> NspNameSpace::getList(const QString &parentPath) const
{
    QList<Entry> result;
    QString prefix = parentPath;
    // Strip "_GLOBALS." prefix for the same reason as findNode
    if (prefix.startsWith("_GLOBALS."))
        prefix = prefix.mid(8);
    prefix = prefix.trimmed();

    // Iterate over all entries; select those whose fullName starts with
    // the prefix and has no additional dots (i.e., are direct children)
    QMap<QString, Entry>::const_iterator it = m_entries.constBegin();
    while (it != m_entries.constEnd()) {
        const Entry &entry = it.value();
        if (!prefix.isEmpty()) {
            // Match entries directly under the given namespace path
            if (entry.fullName.startsWith(prefix + ".") &&
                entry.fullName.indexOf('.', prefix.length() + 1) == -1) {
                // Avoid duplicates in the result list
                bool found = false;
                for (const Entry &existing : result) {
                    if (existing.name == entry.name) { found = true; break; }
                }
                if (!found) result.append(entry);
            }
        } else {
            // No prefix: return all top-level entries (no dots in fullName)
            if (!entry.fullName.contains('.')) {
                bool found = false;
                for (const Entry &existing : result) {
                    if (existing.name == entry.name) { found = true; break; }
                }
                if (!found) result.append(entry);
            }
        }
        ++it;
    }

    // Always include universal methods available on all objects
    static const QStringList universal = {"gettype", "length", "tostring"};
    for (const QString &name : universal) {
        bool found = false;
        for (const Entry &existing : result) {
            if (existing.name == name) { found = true; break; }
        }
        if (!found) {
            Entry e;
            e.name = name;
            e.fullName = prefix.isEmpty() ? name : prefix + "." + name;
            e.type = "function";
            e.desc = "";
            e.params = "";
            e.returns = "";
            result.append(e);
        }
    }

    // Sort alphabetically by name for consistent display
    std::sort(result.begin(), result.end(),
        [](const Entry &a, const Entry &b) { return a.name < b.name; });

    return result;
}