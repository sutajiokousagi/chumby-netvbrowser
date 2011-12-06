#include <unistd.h>
#include <QtGui/QApplication>
#include <qtsingleapplication.h>
#include "mainwindow.h"
#include "stdio.h"
#include <QDebug>
#include <QProcess>
#include <QStringList>

#ifdef ENABLE_QWS_STUFF
    #include <QWSServer>
#endif

bool isRunning()
{
    QProcess *newProc = new QProcess();
    newProc->start("/bin/pidof", QStringList(APPNAME));
    newProc->waitForFinished();

    QByteArray buffer = newProc->readAllStandardOutput();
    newProc->close();
    delete newProc;
    newProc = NULL;

    QStringList pidList = QString(buffer.trimmed()).split(" ", QString::SkipEmptyParts);
    if (pidList.length() > 1)
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    QtSingleApplication instance(argc, argv, QApplication::GuiServer);
    instance.setApplicationName(APPNAME);
    instance.setOrganizationName(ORGANISATION);

    QStringList argsList = instance.arguments();
    QString argsString = argsList.join(ARGS_SPLIT_TOKEN);

//Hide mouse cursor by default
#ifdef ENABLE_QWS_STUFF
    QWSServer *qserver = QWSServer::instance();
    qserver->setCursorVisible(false);
#endif

    // Show help message
    if (argsList.contains("-h") || argsList.contains("--help")) {
        printf("\tUsage: %s [-d] -qws -nomouse [SetUrl http://example.com/]\n"
               "\tIf -d is specified, this program will run as a daemon.\n"
               "\tFor more info, see http://wiki.chumby.com/index.php/NeTV_local_UI\n",
               argv[0]);
        return 0;
    }

    //Check if another instance is already running & attempt to send arguments to it
    if (instance.sendMessage(argsString))
    {
        printf("Sending arguments to running %s instance: %s\n", TAG, argsString.toLatin1().constData());
        return 0;
    }

    //Give it another go
    if (instance.sendMessage(argsString))
    {
        printf("Sending arguments to running %s instance: %s\n", TAG, argsString.toLatin1().constData());
        return 0;
    }

    bool running = isRunning();
    if (running)
    {
        // For some reason, the local socket in previous instance doesn't accept command. We give up.
        printf("Failed to send arguments to running %s instance: %s\n", TAG, argsString.toLatin1().constData());
        return 1;
    }

    // If the args list contains "-d", then daemonize it
    if (argsList.contains("-d") || argsList.contains("--daemon"))
        daemon(0, 0);

    printf("Starting new %s with args:", TAG);
    printf("%s", argsString.toLatin1().constData());

//Pink background for the entire QWS environment
#ifdef ENABLE_QWS_STUFF
    qserver->setBackground(QBrush(QColor(240,0,240)));
#endif

    MainWindow w;
    w.receiveArgs(argsString);
#ifdef Q_WS_QWS
    w.showFullScreen();
#else
    w.show();
#endif
    instance.setActivationWindow(&w);

    QObject::connect(&instance, SIGNAL(messageReceived(const QString&)), &w, SLOT(receiveArgs(const QString&)));

    return instance.exec();
}
