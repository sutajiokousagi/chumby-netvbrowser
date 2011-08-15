#ifndef ASYNC_FIFO_H
#define ASYNC_FIFO_H

#include <QThread>
#include <QString>
#include <QFile>

class async_fifo : public QThread
{
    Q_OBJECT

public:

    explicit async_fifo(QString filename, QObject *parent = 0);

public:

    void stopMe();
    bool isOpen();

protected:

    void run();

private:

    QString filename;
    QFile *file;
    bool isStopping;
    char buf[1024];

    bool setup();
    void readFile();

signals:

    void signal_fileopen(bool);
    void signal_newline(QByteArray);

};

#endif // ASYNC_FIFO_H
