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
        //All these should already be URI encoded
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

    QStringList argsList = QString(dataString).split(ARGS_SPLIT_TOKEN);
    QByteArray string = processStatelessCommand(command, argsList);
    response->setStatus(1);
    //response->setCommand(command);    //this will cause a echo
    response->setParameter("value", string);
    response->write();

    if (request->getAddress().length() > 3)     qDebug("%s: slot_newSocketMessage: %s from %s", TAG, string.constData(), request->getAddress().constData());
    else                                        qDebug("%s: slot_newSocketMessage: %s", TAG, string.constData() );
}
