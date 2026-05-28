// Settings.cpp - Implementation of the Settings class using QSettings
// with organization "NullLogic" and application name "qtnspedit".

#include "Settings.h"

static const QString ORG = "NullLogic";
static const QString APP = "qtnspedit";

// Saves window position/size under the "MainWindow" group.
void Settings::saveWindowGeometry(const QRect &geometry)
{
    QSettings settings(ORG, APP);
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", geometry);
    settings.endGroup();
}

// Loads the saved window geometry; defaults to 800x600 at position (100,100).
QRect Settings::loadWindowGeometry()
{
    QSettings settings(ORG, APP);
    settings.beginGroup("MainWindow");
    QRect geometry = settings.value("geometry", QRect(100, 100, 800, 600)).toRect();
    settings.endGroup();
    return geometry;
}

// Saves the output panel height in the "MainWindow" group.
void Settings::saveOutputHeight(int height)
{
    QSettings settings(ORG, APP);
    settings.beginGroup("MainWindow");
    settings.setValue("outputHeight", height);
    settings.endGroup();
}

// Loads the saved output panel height; defaults to 75 pixels.
int Settings::loadOutputHeight()
{
    QSettings settings(ORG, APP);
    settings.beginGroup("MainWindow");
    int height = settings.value("outputHeight", 75).toInt();
    settings.endGroup();
    return height;
}

// Saves the last directory used in file dialogs under the "Paths" group.
void Settings::saveLastDir(const QString &dir)
{
    QSettings settings(ORG, APP);
    settings.beginGroup("Paths");
    settings.setValue("lastDir", dir);
    settings.endGroup();
}

// Loads the last directory; defaults to empty string.
QString Settings::loadLastDir()
{
    QSettings settings(ORG, APP);
    settings.beginGroup("Paths");
    QString dir = settings.value("lastDir", QString()).toString();
    settings.endGroup();
    return dir;
}