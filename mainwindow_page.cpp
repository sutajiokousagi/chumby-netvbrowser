#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebPage>
#include <QWebFrame>
#include <QDebug>

void MainWindow::slot_pageloadStarted()
{

}

void MainWindow::slot_pageloadProgress(int progress)
{
    if (!cPanelLoaded)
        qDebug("%s: loading ControlPanel...%d%%", TAG, progress);

    //if (progress > 99)
    //    qDebug("%s", qPrintable(this->myWebView->page()->mainFrame()->toHtml()));
}

void MainWindow::slot_pageloadFinished(bool ok)
{
    cPanelLoadCount++;
    if (!cPanelLoaded)
    {
        if (!ok)
        {
            if (cPanelLoadCount < 3)    qDebug("%s: ControlPanel loaded with ERROR! Retrying...", TAG);
            else                        qDebug("%s: ControlPanel loaded with ERROR! Give up after 3 tries!", TAG);

            if (cPanelLoadCount >= 3) {
                cPanelLoaded = true;
                return;
            }

            this->initWebViewTab(DEFAULT_TAB);
            this->resetWebViewTab(DEFAULT_TAB);
            this->showWebViewTab(DEFAULT_TAB);
            return;
        }

        qDebug("%s: ControlPanel loaded OK", TAG);
        cPanelLoaded = true;
    }

    //Hide scrollbars of all the frames
    /*
    qDebug("%s: slot_pageloadFinished 1", TAG);
    this->myWebView->page()->currentFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->currentFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    */

    //Start or restart keep alive timer
    qDebug("%s: slot_pageloadFinished (%d bytes)", TAG, this->myWebView->page()->bytesReceived());
    if (this->enKeepAliveTimer && !this->keepAliveTimer.isActive())
        this->keepAliveTimer.start();

    //Start or restart input focus timer
    if (!this->focusInputTimer.isActive())
        this->focusInputTimer.start();
}

//--------------------------------------------------------------------------------------------------------

void MainWindow::slot_frameCreated(QWebFrame *frame)
{
    if (frame == NULL)
        return;
    //qDebug("%s: new web frame created", TAG);
    frame->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    frame->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
}

//--------------------------------------------------------------------------------------------------------

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
