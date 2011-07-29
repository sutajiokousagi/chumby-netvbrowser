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
}

void MainWindow::slot_pageloadProgress(int progress)
{
    if (!cPanelLoaded)
        qDebug("%s: loading ControlPanel...%d%%", TAG, progress);
}
