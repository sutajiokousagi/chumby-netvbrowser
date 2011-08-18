#include "async_fifo.h"
#include <QDebug>

async_fifo::async_fifo(QString filename, QObject *parent) :
    QThread(parent)
{
    this->file = NULL;
    this->filename = filename;
    this->isStopping = false;
}

bool async_fifo::setup()
{
    if (this->filename == "")
        return false;

    //clean up
    if (this->file != NULL)
    {
        file->close();
        delete file;
        file = NULL;
    }

    if (this->isStopping == true)
        return false;

    this->file = new QFile(filename);
    if (!file->exists())
        return false;

    //This is blocking, should be called in run() function
    bool ok = this->file->open(QIODevice::ReadOnly);
    if (!ok || this->file == NULL || !this->file->isOpen() || !this->file->isReadable())
        return false;

    return true;
}

void async_fifo::stopMe()
{
    this->isStopping = true;
}

bool async_fifo::isOpen()
{
    return (this->file != NULL && this->file->isOpen() && this->file->isReadable());
}

void async_fifo::run()
{
    bool ok = this->setup();
    //if (ok)         qDebug("%s: listening to opkg fifo", "NeTVBrowser");
    //else            qDebug("%s: failed to listen to opkg fifo", "NeTVBrowser");
    msleep(50);
    emit signal_fileopen(ok);
    if (!ok)
        return;

    while (this->isStopping == false)
    {
        QThread::msleep(1000);
        readFile();
    }
    return;
}

void async_fifo::readFile()
{
    while (this->isStopping == false)
    {
        qint64 lineLength = this->file->readLine(this->buf, sizeof(this->buf));
        if (lineLength <= 0)
            break;
        emit signal_newline(QByteArray(buf));
    }
    this->file->close();
    this->file = NULL;
    delete this;
}
