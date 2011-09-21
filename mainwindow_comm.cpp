#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QTimer>
#include <QWebFrame>

void MainWindow::setupSocket()
{
    //clean up
    if (this->mySocket != NULL)
    {
        mySocket->close();
        delete mySocket;
        mySocket = NULL;
    }

    //create new socket
    this->port = DEFAULT_PORT;
    this->mySocket = new QTcpSocket(this);
    connect(this->mySocket, SIGNAL(connected()), this, SLOT(slot_socketConnected()));
    connect(this->mySocket, SIGNAL(disconnected()), this, SLOT(slot_socketDisconnected()));
    connect(this->mySocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slot_socketError(QAbstractSocket::SocketError)));

    //make the connection
    this->mySocket->connectToHost(QString(DEFAULT_HOST_URL), this->port);
}

void MainWindow::slot_socketConnected()
{
    qDebug("%s: connected to NeTVServer", TAG);

    //Notify ControlPanel about this
    if (ENABLE_FSERVERRESET)
        QTimer::singleShot( 400, this, SLOT(slot_notifyBrowser()) );

    QTcpSocket *socket = (QTcpSocket *)QObject::sender();
    connect(socket, SIGNAL(readyRead()), this, SLOT(slot_socketReadReady()));

    SocketResponse *response = new SocketResponse(socket, socket->peerAddress().toString().toLatin1(), socket->peerPort());
    sendSocketHello(response);
    delete response;
}

void MainWindow::slot_socketDisconnected()
{
#ifdef SUPERVERBOSE
    if (QObject::sender() != NULL)
        qDebug("TcpSocket (%x): disconnected",(unsigned int) this);
#endif

    if (QObject::sender() != NULL)
        qDebug("%s: disconnected from NeTVServer", TAG);
    QTimer::singleShot( 3000, this, SLOT(slot_socketRetry()) );
}

void MainWindow::slot_socketError(QAbstractSocket::SocketError err)
{
#ifdef SUPERVERBOSE
    qDebug("TcpSocket (%x): error (%d)", (unsigned int) this, (unsigned int) err);
#else
    Q_UNUSED (err);
#endif

    QTimer::singleShot( 3000, this, SLOT(slot_socketRetry()) );
}

void MainWindow::slot_socketRetry()
{
    this->mySocket->close();
    disconnect(this->mySocket, SIGNAL(connected()), this, SLOT(slot_socketConnected()));
    disconnect(this->mySocket, SIGNAL(disconnected()), this, SLOT(slot_socketDisconnected()));
    disconnect(this->mySocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slot_socketError(QAbstractSocket::SocketError)));
    disconnect(this->mySocket, SIGNAL(readyRead()), this, SLOT(slot_socketReadReady()));

    if (this->isShuttingDown)
        return;

    setupSocket();
}

void MainWindow::slot_socketReadReady()
{
    QTcpSocket *socket = (QTcpSocket *)QObject::sender();

    QByteArray buffer;
    while (socket->bytesAvailable() > 0)
        buffer.append( socket->read(socket->bytesAvailable()) );

    SocketRequest *request = new SocketRequest(buffer, socket->peerAddress().toString().toLatin1(), socket->peerPort());
    if (request->hasError())
    {
        buffer = QByteArray();
        delete request;
        return;
    }

    SocketResponse *response = new SocketResponse(socket, socket->peerAddress().toString().toLatin1(), socket->peerPort());
    this->slot_newSocketMessage(request, response);
    buffer = QByteArray();
    delete response;
    delete request;
}

void MainWindow::slot_notifyBrowser()
{
    //If ControlPanel has loaded & running
    if (cPanelLoaded)
    {
        QString javascriptString = QString("fServerReset();");
        qDebug("%s: calling JavaScript function '%s'", TAG, javascriptString.toLatin1().constData());
        if (HasJavaScriptFunction("fServerReset"))
            this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
        else
            qDebug("%s: does not contain JavaScript function 'fServerReset'", TAG);
    }
}

void MainWindow::slot_newSocketMessage( SocketRequest *request, SocketResponse *response )
{
    QByteArray command = request->getCommand().toUpper();
    QByteArray dataString = request->getParameter("value").trimmed();

    //Some commands are more complex with multiple parameters
    if (command == "TICKEREVENT")
    {
        //All these should already be URI encoded by NeTVServer
        QByteArray message = request->getParameter("message");
        QByteArray title = request->getParameter("title");
        QByteArray image = request->getParameter("image");
        QByteArray type = request->getParameter("type");
        QByteArray level = request->getParameter("level");
        QByteArray javaScriptString = QByteArray("fTickerEvents(\"") + message + "\",\"" + title + "\",\"" + image + "\",\"" + type + "\",\"" + level + "\");";

        //Translate to a JavaScript command
        command = "JAVASCRIPT";
        dataString = javaScriptString;
    }
    else if (command == "SETIFRAME" || command == "MULTITAB")
    {
        QByteArray options = request->getParameter("options");
        QByteArray param = request->getParameter("param");
        QByteArray tab = request->getParameter("tab");
        int tabIndex = tab.toInt();

        if (options.length() < 1 || options.toUpper() == "LOAD")
        {
            if (tabIndex < 1 || tabIndex >= MAX_TABS)
                tabIndex = 1;
            if (param.length() > 5)
            {
                hideWebViewTab(DEFAULT_TAB);
                loadWebViewTab(tabIndex, param);
                qDebug("%s: loading another tab with url [%s]", TAG, param.constData());
            }
            else
            {
                qDebug("%s: not loading invalid url [%s]", TAG, param.constData());
            }
        }
        else if (options.toUpper() == "IMAGE")
        {
            if (tabIndex < 1 || tabIndex >= MAX_TABS)
                tabIndex = 1;
            QString htmlString = QString(HTML_IMAGE);
            htmlString = htmlString.replace("xxxxxxxxxx", QString(param));

            hideWebViewTab(DEFAULT_TAB);
            loadWebViewTabHTML(tabIndex, htmlString.toLatin1());
            qDebug("%s: loading another tab with image [%s]", TAG, param.constData());
        }
        else if (options.toUpper() == "HTML")
        {
            if (tabIndex < 1 || tabIndex >= MAX_TABS)
                tabIndex = 1;
            hideWebViewTab(DEFAULT_TAB);
            loadWebViewTabHTML(tabIndex, QUrl::fromPercentEncoding(param.replace("+", " ")).toLatin1() );
            qDebug("%s: loading another tab with HTML string", TAG);
            qDebug("%s", QUrl::fromPercentEncoding(param.replace("+", " ")).toLatin1().constData());
        }
        else if (options.toUpper() == "HIDE" || options.toUpper() == "HIDEALL" || options.toUpper() == "BACK")
        {
            hideOtherWebViewTab(DEFAULT_TAB);
            showWebViewTab(DEFAULT_TAB);
        }
        else if (options.toUpper() == "HIDEONE")
        {
            hideWebViewTab(tabIndex);
        }
        else if (options.toUpper() == "SCROLL")
        {
            QList<QByteArray> args = param.split(',');
            if (args.size() >= 2)
            {
                bool okx = false;
                bool oky = false;
                int x = args.at(0).toInt(&okx);
                int y = args.at(1).toInt(&oky);
                if (okx && oky)
                    scrollWebViewTabAbsolute(this->currentWebViewTab, x,y);
            }

            //No need to notify Control Panel
            return;
        }
        else if (options.toUpper() == "SCROLLF")
        {
            QList<QByteArray> args = param.split(',');
            if (args.size() >= 2)
            {
                bool okx = false;
                bool oky = false;
                double x = args.at(0).toDouble(&okx);
                double y = args.at(1).toDouble(&oky);
                if (okx && oky)
                    scrollWebViewTabPercentage(this->currentWebViewTab, x,y);
                else
                    qDebug("%s: float conversion failed %s", TAG, param.constData());
            }

            //No need to notify Control Panel
            return;
        }

        //Translate to a JavaScript command

        param = QUrl::toPercentEncoding(param, "", "/'\"");
        QByteArray javaScriptString = QByteArray("fMultitab(\"") + options + "\",\"" + param + "\",\"" + tab + "\");";
        command = "JAVASCRIPT";
        dataString = javaScriptString;
    }

    QStringList argsList = QString(dataString).split(ARGS_SPLIT_TOKEN);
    QByteArray string = processStatelessCommand(command, argsList);
    response->setStatus(1);
    //response->setCommand(command);    //this will cause a echo if called from NeTVServer
    response->setParameter("value", string);
    response->write();

    if (request->getAddress().length() > 3)     qDebug("%s: slot_newSocketMessage: %s from %s", TAG, string.constData(), request->getAddress().constData());
    else                                        qDebug("%s: slot_newSocketMessage: %s", TAG, string.constData() );
}
