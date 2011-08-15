#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QProcess>

void MainWindow::setupUpgrade()
{
    //clean up
    if (this->opkgFifo != NULL)
    {
        opkgFifo->stopMe();
        QObject::disconnect(opkgFifo, SIGNAL(signal_fileopen(bool)), this, SLOT(slot_opkgFileOpen(bool)));
        QObject::disconnect(opkgFifo, SIGNAL(signal_newline(QByteArray)), this, SLOT(slot_opkgNewLine(QByteArray)));
        delete opkgFifo;
        opkgFifo = NULL;
    }

    //Create the fifo if it doesn't exist yet
    if (!FileExists(OPKG_FIFO))
        QProcess::startDetached("mkfifo", QStringList(QString(OPKG_FIFO)));

    //Start an async fifo reader
    this->opkgFifo = new async_fifo(QString(OPKG_FIFO), this);
    QObject::connect(opkgFifo, SIGNAL(signal_fileopen(bool)), this, SLOT(slot_opkgFileOpen(bool)));
    QObject::connect(opkgFifo, SIGNAL(signal_newline(QByteArray)), this, SLOT(slot_opkgNewLine(QByteArray)));
    this->opkgFifo->start();
}

void MainWindow::resetUpgrade()
{
    packageList.clear();
    packageSizeMap.clear();
    packageStateMap.clear();
}

void MainWindow::slot_opkgFileOpen(bool isOpen)
{
    if (isOpen)     qDebug("%s: listening to opkg fifo", TAG);
    else            qDebug("%s: failed to listen to opkg fifo", TAG);
}

void MainWindow::slot_opkgNewLine(QByteArray newline)
{
    qDebug() << "---";
    qDebug() << newline;

    //Indicate that this package is done
    updatePackageState(newline, true);

    //Execute JavaScript
    QString javascriptString = QString("fUPDATEEvents('progress',%1);").arg(QString().setNum(getUpdatePercentage()));
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    qDebug() << "Upgrade progress: " << getUpdatePercentage() << "%";

    //Clean up memory
    newline = QByteArray();
}

//-----------------------------------------------------------------------------------
// Getting useful info
//-----------------------------------------------------------------------------------

qint64 MainWindow::doUpgrade()
{
    qint64 pid = 0;
    QProcess::startDetached(QString(UPGRADE_SCRIPT), QStringList(), QString(""), &pid);
    return pid;
}

void MainWindow::upgradeDone()
{
    if (opkgFifo != NULL) {
        opkgFifo->stopMe();
    }

    //Execute JavaScript
    QString javascriptString = QString("fUPDATEEvents('done', '');");
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    qDebug() << "Upgrade done!";
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
