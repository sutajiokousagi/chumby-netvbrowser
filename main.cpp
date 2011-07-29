#include <unistd.h>
#include <QtGui/QApplication>
#include <qtsingleapplication.h>
#include "mainwindow.h"
#include "stdio.h"

#ifdef ENABLE_QWS_STUFF
    #include <QWSServer>
#endif

int main(int argc, char *argv[])
{
    //Check if another instance is already running by sending a message to it
    QtSingleApplication instance(argc, argv, QApplication::GuiServer);
    instance.setApplicationName(APPNAME);
    instance.setOrganizationName(ORGANISATION);

    QStringList argsList = instance.arguments();
    QString argsString = argsList.join(ARGS_SPLIT_TOKEN);

    if (instance.sendMessage(argsString))
    {
        printf("Sending arguments to running NeTVBrowser instance: %s\n", argsString.toLatin1().constData());
        return 0;
    }

    if (argsList.contains("-h")) {
        printf("\tUsage: %s [-d] -qws -nomouse [SetUrl http://example.com/]\n"
               "\tIf -d is specified, this program will run as a daemon.\n",
               argv[0]);
        return 0;
    }

    // If the args list contains "-d", then daemonize
    if (argsList.contains("-d"))
        daemon(0, 0);

    //Pink background (temp fix. We should completely prevent it from drawing in non-window area)
#ifdef ENABLE_QWS_STUFF
    QWSServer *qserver = QWSServer::instance();
    qserver->setBackground(QBrush(QColor(240,0,240)));
    qserver->setCursorVisible(false);
#endif

    printf("Starting new %s with args:", TAG);
    printf("%s", argsString.toLatin1().constData());

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
