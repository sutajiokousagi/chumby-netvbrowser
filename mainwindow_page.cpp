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

    //Hide scrollbars of all the frames
    this->myWebView->page()->currentFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->currentFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Start or restart keep alive timer
    if (!this->keepAliveTimer.isActive())
        this->keepAliveTimer.start();
}

void MainWindow::slot_pageloadProgress(int progress)
{
    if (!cPanelLoaded)
        qDebug("%s: loading ControlPanel...%d%%", TAG, progress);
}

//--------------------------------------------------------------------------------------------------------

void MainWindow::slot_frameCreated(QWebFrame *frame)
{
    //qDebug("%s: new web frame created", TAG);
    frame->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    frame->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    QObject::connect(frame, SIGNAL(contentsSizeChanged(QSize)), this, SLOT(slot_frameContentSizeChange(QSize)));
}

void MainWindow::slot_frameLoadFinished(bool)
{
    QWebFrame *frame = (QWebFrame *)QObject::sender();
    if (frame == NULL)
        return;

    qDebug("%s: myIFrame loade finished (%d x %d)", TAG, frame->contentsSize().width(), frame->contentsSize().height());
    myWebView->page()->mainFrame()->evaluateJavaScript( QString("fSetIFrame(\"resize\", \"%1,%2\");").arg(frame->contentsSize().width()).arg(frame->contentsSize().height()) );
    frame->setScrollPosition(QPoint(0,0));
}

void MainWindow::slot_frameContentSizeChange(const QSize&)
{
    QWebFrame *frame = (QWebFrame *)QObject::sender();
    if (frame == NULL)
        return;

    //This will remember the iFrame element when we first see it
    if ((frame->frameName() == "iframe_externalUrlPlayer" || frame->requestedUrl().toString() == "about:blank") && myIFrame == NULL)
    {
        myIFrame = frame;
        QObject::connect(myIFrame, SIGNAL(loadFinished(bool)), this, SLOT(slot_frameLoadFinished(bool)));
    }

    if (frame != myIFrame || myIFrame == NULL)
        return;

    //Resize iFrame whenever it changes (can be quite frequent as new content is loading)
    qDebug("%s: myIFrame size changed (%d x %d)", TAG, frame->contentsSize().width(), frame->contentsSize().height());
    if (HasJavaScriptFunction("fSetIFrame"))
        myWebView->page()->mainFrame()->evaluateJavaScript( QString("fSetIFrame(\"resize\", \"%1,%2\");").arg(frame->contentsSize().width()).arg(frame->contentsSize().height()) );
    else
        qDebug("%s: does not contain JavaScript function 'fSetIFrame' (yet?)", TAG);

    //if (HasJavaScriptFunction("dasgrqaew"))         qDebug("%s: does contain JavaScript function 'dasgrqaew'", TAG);
    //else                                            qDebug("%s: does not contain JavaScript function 'dasgrqaew'", TAG);
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
