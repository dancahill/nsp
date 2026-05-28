// Settings.h - Static wrapper around QSettings for persisting application state.
// Saves/loads window geometry, output panel height, and last visited directory.
// Uses organization "NullLogic" and app name "qtnspedit".

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QRect>

class Settings {
public:
    // Saves the main window position/size under the "MainWindow" group.
    static void saveWindowGeometry(const QRect &geometry);
    // Loads the saved window geometry; defaults to 800x600 at (100,100).
    static QRect loadWindowGeometry();

    // Saves the height of the output panel in the splitter.
    static void saveOutputHeight(int height);
    // Loads the saved output height; defaults to 75 pixels.
    static int loadOutputHeight();

    // Saves the last directory used in file open/save dialogs.
    static void saveLastDir(const QString &dir);
    // Loads the last directory; defaults to empty string.
    static QString loadLastDir();
};

#endif // SETTINGS_H