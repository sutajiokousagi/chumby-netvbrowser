#include <QtGui/QApplication>
#include <qtsingleapplication.h>
#include "mainwindow.h"
#include "stdio.h"

int main(int argc, char *argv[])
{
    //Check if another instance is already running by sending a message to it
    QtSingleApplication instance(argc, argv);
    QStringList argsList = instance.arguments();
    QString argsString = argsList.join(ARGS_SPLIT_TOKEN);

    if (instance.sendMessage(argsString))
    {
        printf("Sending '%s' to running NeTVBrowser instance", argsString.toLatin1().constData());
        return 0;
    }

    printf("Starting new NeTVBrowser with args:");
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
