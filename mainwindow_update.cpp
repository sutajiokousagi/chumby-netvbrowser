#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QFileInfoList>
#include <QProcess>

void MainWindow::setupUpdate()
{
    //clean up
    if (this->opkgSocket != NULL)
    {
        opkgSocket->close();
        delete opkgSocket;
        opkgSocket = NULL;
    }

    //Make sure mkfifo is called in NeTVBrowser start up script
    this->opkgSocket = new QLocalServer(this);

    if (FileExists(OPKG_FIFO))
        UnlinkFile(OPKG_FIFO);
    bool ok = opkgSocket->listen(OPKG_FIFO);

    if (ok)     qDebug("%s: listerning to opkg fifo", TAG);
    else        qDebug("%s: failed to listen to opkg fifo (%s)", TAG, opkgSocket->errorString().toLatin1().constData());

    if (ok)
        QObject::connect(opkgSocket, SIGNAL(newConnection()), SLOT(slot_opkgReceiveConnection()));
}

void MainWindow::resetUpdate()
{
    packageList.clear();
    packageSizeMap.clear();
    packageStateMap.clear();
}

void MainWindow::slot_opkgConnected()
{
    qDebug("%s: connected to opkg fifo", TAG);

    QLocalSocket *socket = (QLocalSocket *)QObject::sender();
    connect(socket, SIGNAL(readyRead()), this, SLOT(slot_opkgReadReady()));
}

void MainWindow::slot_opkgDisconnected()
{
    qDebug("%s: disconnected from opkg fifo", TAG);

    if (QObject::sender() == NULL)
        return;

    QLocalSocket *socket = (QLocalSocket *)QObject::sender();
    socket->close();
    QObject::disconnect(socket, SIGNAL(connected()), this, SLOT(slot_opkgConnected()));
    QObject::disconnect(socket, SIGNAL(disconnected()), this, SLOT(slot_opkgDisconnected()));
    QObject::disconnect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(slot_opkgError(QLocalSocket::LocalSocketError)));
    QObject::disconnect(socket, SIGNAL(readyRead()), this, SLOT(slot_opkgReadReady()));
}

void MainWindow::slot_opkgError(QLocalSocket::LocalSocketError err)
{
    Q_UNUSED(err);
    qDebug() << err;
}

void MainWindow::slot_opkgReadReady()
{
    QLocalSocket *socket = (QLocalSocket *)QObject::sender();

    //Should be a line here
    QByteArray buffer;
    while (socket->bytesAvailable() > 0)
        buffer.append( socket->read(socket->bytesAvailable()) );
    qDebug() << "---";
    qDebug() << buffer;

    //Indicate that this package is done
    updatePackageState(buffer, true);

    //Execute JavaScript
    QString javascriptString = QString("fUPDATEEvents('progress',%1);").arg(QString().setNum(getUpdatePercentage()));
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    qDebug() << "Upgrade progress: " << getUpdatePercentage() << "%";
}

void MainWindow::slot_opkgReceiveConnection()
{
    QLocalSocket* socket = this->opkgSocket->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, SIGNAL(connected()), this, SLOT(slot_opkgConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slot_opkgDisconnected()));
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(slot_opkgError(QLocalSocket::LocalSocketError)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(slot_opkgReadReady()));

    /*
    while (socket->bytesAvailable() < (int)sizeof(quint32))
        socket->waitForReadyRead();
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));
    if (got < 0) {
        qWarning() << "QtLocalPeer: Message reception failed" << socket->errorString();
        delete socket;
        return;
    }
    QString message(QString::fromUtf8(uMsg));
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    delete socket;
    emit messageReceived(message); //### (might take a long time to return)
    */
}


//-----------------------------------------------------------------------------------
// Getting useful info
//-----------------------------------------------------------------------------------

qint64 MainWindow::doUpgrade()
{
    qint64 pid = 0;
    QProcess::startDetached(QString("/usr/bin/opkg"), QStringList("upgrade"), QString(""), &pid);
    return pid;
}

QByteArray MainWindow::getFriendlyPackageName(QByteArray rawName)
{
    //netv-utils - 0-r39.9 - 0-r40.9
    //angstrom-version - v20110703-r22.9 - v20110703-r24.9
    return rawName.split('-')[0].trimmed();
}

void MainWindow::getUpgradablePackageList()
{
    QByteArray buffer = this->Execute("/usr/bin/opkg", QStringList("list-upgradable"));

    this->packageList = buffer.split('\n');
    for( int i=0; i<packageList.size(); i++ )
    {
        packageSizeMap.insert(packageList[i], 1);
        packageStateMap.insert(packageList[i], false);
    }
}

void MainWindow::getDownloadedPackageSize()
{
    QDir myDir(OPKG_DOWNLOAD_PATH);
    QFileInfoList listinfolist = myDir.entryInfoList(QStringList("*.ipk"));
    for (int i=0; i<listinfolist.size(); i++)
    {
        QFileInfo fileinfo = listinfolist.at(i);
        updatePackageSize( fileinfo.filePath().toLatin1(), fileinfo.size() );
    }
}

void MainWindow::updatePackageSize(QByteArray packagefilename, quint64 size)
{
    QMapIterator<QByteArray, quint64> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();

        //angstrom-version - v20110703-r22.9 - v20110703-r24.9
        //...n-netv-debug,LATEST,chumby-silvermoon-netv,angstrom-version_v20110703-r24.9_chumby-silvermoon-netv.ipk
        QByteArray filename = i.key();
        filename.replace(" - ", "_");

        if (!packagefilename.contains(filename))
            continue;
        packageSizeMap.insert(i.key(), size);
        qDebug() << QString(i.key()) << " --> " << size;
        break;
    }
}

void MainWindow::updatePackageState(QByteArray packagefilename, bool isDone)
{
    QMapIterator<QByteArray, bool> i(packageStateMap);
    while (i.hasNext())
    {
        i.next();

        //angstrom-version - v20110703-r22.9 - v20110703-r24.9
        //...n-netv-debug,LATEST,chumby-silvermoon-netv,angstrom-version_v20110703-r24.9_chumby-silvermoon-netv.ipk
        QByteArray filename = i.key();
        filename.replace(" - ", "_");

        if (!packagefilename.contains(filename))
            continue;
        packageStateMap.insert(i.key(), isDone);
        break;
    }
}

quint64 MainWindow::getUpdateTotalSizeKb()
{
    quint64 size = 0;
    QMapIterator<QByteArray, quint64> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();
        size += i.value();
    }
    return size;
}

quint64 MainWindow::getUpdateProgressSizeKb()
{
    quint64 size = 0;
    QMapIterator<QByteArray, bool> i(packageStateMap);
    while (i.hasNext())
    {
        i.next();
        if (i.value() == false)
            continue;
        size += packageSizeMap[i.key()];
    }
    return size;
}

double MainWindow::getUpdatePercentage()
{
    return (double)getUpdateProgressSizeKb() / getUpdateTotalSizeKb() * 100.0;
}
