QT       += core gui widgets xml network

TARGET    = qtnspedit
TEMPLATE  = app
CONFIG    += c++17 silent
DEFINES   += QT_NO_DEPRECATED_WARNINGS

CONFIG    -= debug_and_release

OBJECTS_DIR = .
MOC_DIR      = .
RCC_DIR      = .
UI_DIR       = .
DESTDIR      = .

INCLUDEPATH += $$PWD/../../../include

win32 {
    LIBS += -L$$PWD/../../../lib -llibnsp
    LIBS += -lshell32
} else {
    LIBS += -L$$PWD/../../../lib -lnsp
}

SOURCES += main.cpp \
           MainWindow.cpp \
           CodeEditor.cpp \
           NspSyntaxHighlighter.cpp \
           NspCompleter.cpp \
           NspNameSpace.cpp \
           ScriptRunner.cpp \
           OutputPanel.cpp \
           FindBar.cpp \
           DebugPanel.cpp \
           Settings.cpp \
           NspFormatter.cpp \
           FileBrowser.cpp

HEADERS += MainWindow.h \
           CodeEditor.h \
           NspSyntaxHighlighter.h \
           NspCompleter.h \
           NspNameSpace.h \
           ScriptRunner.h \
           OutputPanel.h \
           FindBar.h \
           DebugPanel.h \
           Settings.h \
           NspFormatter.h \
           FileBrowser.h

RESOURCES += resources/qtnspedit.qrc

win32:RC_FILE = resources/qtnspedit.rc