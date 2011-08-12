#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>


//-----------------------------------------------------------------------------------------------------------
// Process Utilities
//-----------------------------------------------------------------------------------------------------------

QByteArray MainWindow::Execute(const QString &fullPath)
{
    return this->Execute(fullPath, QStringList());
}

QByteArray MainWindow::Execute(const QString &fullPath, QStringList args)
{
    if (!SetFileExecutable(fullPath))
        return QByteArray("no permission to execute");

    QProcess *newProc = new QProcess();
    newProc->start(fullPath, args);
    newProc->waitForFinished();

    // Print the output to HTML response & Clean up
    QByteArray buffer = newProc->readAllStandardOutput();
    newProc->close();
    delete newProc;
    newProc = NULL;

    return buffer;
}


//-----------------------------------------------------------------------------------------------------------
// File Utilities
//-----------------------------------------------------------------------------------------------------------

bool MainWindow::FileExists(const QString &fullPath)
{
    QFile file(fullPath);
    return file.exists();
}

bool MainWindow::FileExecutable(const QString &fullPath)
{
    QFile file(fullPath);
    if (!file.exists())
        return false;
    return ((file.permissions() & QFile::ExeOwner) || (file.permissions() & QFile::ExeOther));
}

qint64 MainWindow::GetFileSize(const QString &fullPath)
{
    QFile file(fullPath);
    if (!file.exists())
        return -1;
    return file.size();
}

QByteArray MainWindow::GetFileContents(const QString &fullPath)
{
    QFile file(fullPath);
    if (!file.exists())                         //doesn't exist
        return "file not found";
    if (!file.open(QIODevice::ReadOnly))        //permission error?
        return "no permission";
    QByteArray buffer = file.readAll();
    file.close();
    return buffer;
}

bool MainWindow::SetFileContents(const QString &fullPath, QByteArray data)
{
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly))       //permission error?
        return false;
    qint64 actual = file.write(data, data.length());
    if (actual != data.length()) {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool MainWindow::UnlinkFile(const QString &fullPath)
{
    QFile file(fullPath);
    if (!file.exists())                         //doesn't exist
        return true;
    return file.remove();
}

bool MainWindow::SetFileExecutable(const QString &fullPath)
{
    QFile file(fullPath);
    if (!file.exists())                         //doesn't exist
        return false;
    if (FileExecutable(fullPath))
        return true;
    return file.setPermissions(file.permissions() | QFile::ExeUser | QFile::ExeOther);
}

