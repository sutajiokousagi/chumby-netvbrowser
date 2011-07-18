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
#ifdef SUPERVERBOSE
    qDebug("TcpSocket (%x): connected",(unsigned int) this);
#endif

    qDebug("NeTVBrowser:slot_socketConnected: connected to NeTVServer");
    static bool firstTime = true;

    //Notify the ControlPanel about this event
    QString javascriptString = QString("fServerReset(%1);").arg(firstTime ? "true" : "false");
    qDebug("NeTVBrowser: calling JavaScript function %s", javascriptString.toLatin1().constData());
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    firstTime = false;

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

    QTimer::singleShot( 2000, this, SLOT(slot_sockerRetry()) );
}

void MainWindow::slot_socketError(QAbstractSocket::SocketError err)
{
#ifdef SUPERVERBOSE
    qDebug("TcpSocket (%x): error (%d)", (unsigned int) this, (unsigned int) err);
#endif

    QTimer::singleShot( 2000, this, SLOT(slot_sockerRetry()) );
}

void MainWindow::slot_sockerRetry()
{
    this->mySocket->close();
    disconnect(this->mySocket, SIGNAL(connected()), this, SLOT(slot_socketConnected()));
    disconnect(this->mySocket, SIGNAL(disconnected()), this, SLOT(slot_socketDisconnected()));
    disconnect(this->mySocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slot_socketError(QAbstractSocket::SocketError)));
    disconnect(this->mySocket, SIGNAL(readyRead()), this, SLOT(slot_socketReadReady()));

    if (isShuttingDown)
        return;

    setupSocket();
}

void MainWindow::slot_socketReadReady()
{
#ifdef SUPERVERBOSE
    qDebug("TcpSocket (%x): read input", (unsigned int) this);
#endif

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

void MainWindow::slot_newSocketMessage( SocketRequest *request, SocketResponse *response )
{
    QByteArray command = request->getCommand().toUpper();
    QByteArray dataString = request->getParameter("value").trimmed();
    QStringList argsList = QString(dataString).split(ARGS_SPLIT_TOKEN);

    QByteArray string = processStatelessCommand(command, argsList);
    response->setStatus(1);
    response->setParameter("value", string);
    response->write();

    qDebug("NeTVBrowser:slot_newSocketMessage: %s", string.constData());
}
