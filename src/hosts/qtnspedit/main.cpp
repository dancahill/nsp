// main.cpp - Application entry point for qtnspedit.
// Creates the QApplication, implements single-instance enforcement using
// QSharedMemory and QLocalServer, opens files passed on the command line,
// and starts the main event loop.

#include "MainWindow.h"

#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QByteArray>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("NullLogic");
    app.setApplicationName("qtnspedit");

    // Single-instance check: if another instance is already running,
    // send it the filenames to open and exit immediately.
    QSharedMemory sharedMem("qtnspedit_single_instance");
    if (!sharedMem.create(1)) {
        // Another instance is running — connect to its local server
        QLocalSocket socket;
        socket.connectToServer("qtnspedit_single_instance");
        if (socket.waitForConnected(500)) {
            // Send each non-flag argument as a file to open
            for (int i = 1; i < argc; i++) {
                QString arg = argv[i];
                if (!arg.startsWith("-")) {
                    socket.write(arg.toUtf8() + "\n");
                }
            }
            socket.waitForBytesWritten();
            return 0;
        }
    }

    MainWindow mainWindow;

    // Local server listens for incoming connections from second instances
    // that want to open files in the already-running first instance.
    QLocalServer *server = new QLocalServer(&app);
    server->listen("qtnspedit_single_instance");
    QObject::connect(server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server->nextPendingConnection();
        if (client->waitForReadyRead(1000)) {
            QByteArray data = client->readAll();
            QString fileName = QString::fromUtf8(data).trimmed();
            if (!fileName.isEmpty())
                mainWindow.loadFile(fileName);
        }
        client->deleteLater();
    });

    // Process command-line arguments: -f flag or bare filenames are opened
    bool fileLoaded = false;
    for (int i = 1; i < argc; i++) {
        QString arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            i++;
            mainWindow.loadFile(argv[i]);
            fileLoaded = true;
        } else if (!arg.startsWith("-")) {
            mainWindow.loadFile(arg);
            fileLoaded = true;
        }
    }

    mainWindow.show();
    return app.exec();
}