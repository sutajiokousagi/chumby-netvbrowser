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

    //Reload previous upgrading progress
    if (FileExists(UPGRADE_PROGRESS_FILE))
    {
        resetUpgrade();
        importPackageList();

        if (packageSizeMap.isEmpty())
        {
            qDebug("%s: Loaded package list is empty", TAG);
            upgradeDone();
            return;
        }
        qDebug("%s: Total upgrade size: %lldMb", TAG, getUpdateTotalSizeKb()/1024);

        //Execute JavaScript
        QString javascriptString = QString("fUPDATEEvents('progress',%1);").arg(QString().setNum(getUpdatePercentage()));
        this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
        qDebug() << "Upgrade progress: " << getUpdatePercentage() << "%";
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
    packageSizeMap.clear();
}

qint64 MainWindow::doUpgrade()
{
    qint64 pid = 0;
    QProcess::startDetached(QString(UPGRADE_SCRIPT), QStringList(), QString(""), &pid);
    return pid;
}

void MainWindow::upgradeDone()
{
    if (opkgFifo != NULL)
        opkgFifo->stopMe();

    //Execute JavaScript
    QString javascriptString = QString("fUPDATEEvents('done', '');");
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    qDebug() << "Upgrade done!";

    //Clean up
    UnlinkFile(UPGRADE_PROGRESS_FILE);
    UnlinkFile(OPKG_FIFO);
}

//--------------------------------------------------------------------

void MainWindow::slot_opkgFileOpen(bool isOpen)
{
    if (isOpen)     qDebug("%s: listening to opkg fifo", TAG);
    else            qDebug("%s: failed to listen to opkg fifo", TAG);
}

void MainWindow::slot_opkgNewLine(QByteArray newline)
{
    //qDebug() << newline;

    //Indicate that this package is done
    updatePackageState(newline, true);

    //Execute JavaScript
    QString javascriptString = QString("fUPDATEEvents('progress',%1);").arg(QString().setNum(getUpdatePercentage()));
    this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString);
    qDebug() << "Upgrade progress: " << getUpdatePercentage() << "%";

    //Clean up memory
    newline = QByteArray();

    //Save package state to a temp file
    exportPackageList();
}

//-----------------------------------------------------------------------------------
// Getting useful info
//-----------------------------------------------------------------------------------

QByteArray MainWindow::getFriendlyPackageName(QByteArray rawName)
{
    //netv-utils - 0-r39.9 - 0-r40.9
    //angstrom-version - v20110703-r22.9 - v20110703-r24.9
    return rawName.split('-')[0].trimmed();
}

void MainWindow::getUpgradablePackageList()
{
    // Example output:
    // chumby-netvserver - 1.0-r76.9 - 1.0-r77.9
    // angstrom-version - v20110703-r18.9 - v20110703-r19.9
    // chumby-netvbrowser - 1.0-r36.9 - 1.0-r37.9
    // netv-controlpanel - 1.0-r30.9 - 1.0-r31.9
    QByteArray buffer = this->Execute("/usr/bin/opkg", QStringList("list-upgradable"));
    QList<QByteArray> packageList = buffer.split('\n');

    for( int i=0; i<packageList.size(); i++ )
        packageSizeMap.insert(packageList[i], QByteArray("false|") + ARGS_SPLIT_TOKEN + "0");
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
    QMapIterator<QByteArray, QByteArray> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();

        // angstrom-version - v20110703-r22.9 - v20110703-r24.9
        // http,,www......n-netv-debug,LATEST,chumby-silvermoon-netv,angstrom-version_v20110703-r24.9_chumby-silvermoon-netv.ipk
        QByteArray filename = i.key();
        filename.replace(" - ", "_");

        if (filename.length() < 3 || !packagefilename.contains(filename))
            continue;

        QByteArray state = packageSizeMap.value(i.key(), "");
        state = QString(state).split(ARGS_SPLIT_TOKEN).at(0).toLatin1();
        QByteArray newMapValue = state + ARGS_SPLIT_TOKEN + QByteArray().setNum(size);

        packageSizeMap.insert(i.key(), newMapValue);
        qDebug() << QString(i.key()) << " --> " << newMapValue;
        break;
    }

    qDebug("%s: Total upgrade size: %lldMb", TAG, getUpdateTotalSizeKb()/1024);
}

void MainWindow::updatePackageState(QByteArray packagefilename, bool isDone)
{
    QMapIterator<QByteArray, QByteArray> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();

        //angstrom-version - v20110703-r22.9 - v20110703-r24.9
        //...n-netv-debug,LATEST,chumby-silvermoon-netv,angstrom-version_v20110703-r24.9_chumby-silvermoon-netv.ipk
        QByteArray filename = i.key();
        filename.replace(" - ", "_");

        if (filename.length() < 3 || !packagefilename.contains(filename))
            continue;

        QByteArray size = packageSizeMap.value(i.key(), "");
        size = QString(size).split(ARGS_SPLIT_TOKEN).at(1).toLatin1();
        QByteArray newMapValue = (isDone ? QByteArray("true") : QByteArray("false")) + ARGS_SPLIT_TOKEN + size;

        packageSizeMap.insert(i.key(), newMapValue);
        break;
    }
}

quint64 MainWindow::getPackageSize(QByteArray packagefilename)
{
    QByteArray packageInfo = packageSizeMap.value(packagefilename, "");
    if (packageInfo.length() < 3)
        return 0;

    QByteArray sizeStr = (QString(packageInfo).split(ARGS_SPLIT_TOKEN)).at(1).toLatin1();
    bool ok = false;
    quint64 size = sizeStr.toULongLong(&ok);

    if (!ok)        return 0;
    else            return size;
}

bool MainWindow::getPackageState(QByteArray packagefilename)
{
    QByteArray packageInfo = packageSizeMap.value(packagefilename, "");
    if (packageInfo.length() < 3)
        return 0;

    QByteArray stateStr = (QString(packageInfo).split(ARGS_SPLIT_TOKEN)).at(0).toLatin1();
    return stateStr.contains("true");
}


quint64 MainWindow::getUpdateTotalSizeKb()
{
    quint64 size = 0;
    QMapIterator<QByteArray, QByteArray> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();
        size += getPackageSize(i.key());
    }
    return size/1024;
}

quint64 MainWindow::getUpdateProgressSizeKb()
{
    quint64 size = 0;
    QMapIterator<QByteArray, QByteArray> i(packageSizeMap);
    while (i.hasNext())
    {
        i.next();
        if (getPackageState(i.key()) == false)
            continue;
        size += getPackageSize(i.key());
    }
    return size/1024;
}

double MainWindow::getUpdatePercentage()
{
    return (double)getUpdateProgressSizeKb() / getUpdateTotalSizeKb() * 100.0;
}

//-----------------------------------------------------------------------------------
// Import/export
//-----------------------------------------------------------------------------------

bool MainWindow::exportPackageList()
{
    QFile file(UPGRADE_PROGRESS_FILE);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_7);
    out << packageSizeMap;
    return true;
}

bool MainWindow::importPackageList()
{
    QFile file(UPGRADE_PROGRESS_FILE);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_7);
    in >> packageSizeMap;
    return true;
}
