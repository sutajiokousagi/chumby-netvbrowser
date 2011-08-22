#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebPage>
#include <QWebFrame>
#include <QDebug>

void MainWindow::slot_pageloadStarted()
{

}

void MainWindow::slot_pageloadFinished(bool ok)
{
    if (!cPanelLoaded)
    {
        cPanelLoaded = true;
        if (ok)     qDebug("%s: ControlPanel loaded", TAG);
        else        qDebug("%s: ControlPanel loaded with ERROR!", TAG);
    }
}

void MainWindow::slot_pageloadProgress(int progress)
{
    if (!cPanelLoaded)
        qDebug("%s: loading ControlPanel...%d%%", TAG, progress);
}

void MainWindow::slot_statusBarMessage ( const QString & text )
{
    //Simple check to see if the message is in XML format
    if (!text.startsWith("<xml>") || !text.endsWith("</xml>"))
    {
        //qDebug("%s: Statusbar: %s", TAG, text.toLatin1().constData());
        return;
    }

    //Parse the XML, check for error
    SocketRequest *request = new SocketRequest(text.toLatin1(), QByteArray(), 0);
    if (request->hasError())
    {
        qDebug("%s: Statusbar: [Error in XML format] %s", TAG, text.toLatin1().constData());
        return;
    }

    //Process the event
    SocketResponse *response = new SocketResponse(NULL, QByteArray(), 0);
    this->slot_newSocketMessage(request, response);
    delete response;
    delete request;
}
