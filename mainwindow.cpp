#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QStringList>
#include <QProcess>
#include <QWebFrame>
#include <QUrl>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->isShuttingDown = false;
    this->cPanelLoaded = false;
    this->myWebView = NULL;
    this->myWebPage = NULL;
    this->mySocket = NULL;
    this->port = DEFAULT_PORT;
    this->opkgFifo = NULL;

    up = 0;down = 0;left = 0;right = 0;center = 0;cpanel = 0;widget = 0;hidden1 = 0; hidden2 = 0;

    ui->setupUi(this);

#ifdef ENABLE_QWS_STUFF
    //Chromeless window
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);     //Set window to fixed size
    this->setWindowFlags(Qt::CustomizeWindowHint);              //Set window with no title bar
    this->setWindowFlags(Qt::FramelessWindowHint);              //Set a frameless window
#endif

    this->setupSocket();
    this->setupWebview();

    //Previously not doing an update
    if (!FileExists(UPGRADE_PROGRESS_FILE))
    {
        this->resetWebview();
        return;
    }

    //Continue reading the fifo
    this->setupUpgrade();

    //Here we have to reload the package list & recalculate the size
    QByteArray progress = "50";
    this->resetWebview(QByteArray(UPDATE_PAGE) + "?continue=" + progress);
}

void MainWindow::setupWebview()
{
    //Remove the existing webview
    this->ui->rootLayout->removeWidget( this->ui->webView );

    //Replace it with our custom webview
    this->myWebView = new MyWebView(this);
    this->myWebPage = new MyWebPage(this);
    this->ui->rootLayout->addWidget(this->myWebView);

    //Ignore mouse & keyboard
    this->myWebView->setEnabled(false);
}

void MainWindow::resetWebview(QByteArray address /* = "" */)
{
    qDebug("NeTVBrowser:resetWebview");

    //Do any other customization on default view state
    this->myWebView->setInvertColor(false);
    this->myWebView->setPage(myWebPage);

    //Hide scrollbars
    this->myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Connect signal
    QObject::connect(this->myWebView->page(), SIGNAL(loadFinished(bool)), this, SLOT(slot_pageloadFinished(bool)));
    QObject::connect(this->myWebView->page(), SIGNAL(loadStarted()), this, SLOT(slot_pageloadStarted()));
    QObject::connect(this->myWebView->page(), SIGNAL(loadProgress(int)), this, SLOT(slot_pageloadProgress(int)));

    //Transparent background (page content dependent)
    QPalette palette = this->myWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    this->myWebView->page()->setPalette(palette);
    this->myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    //Default background color (bright pink chroma key color)
    this->setStyleSheet("background-color: rgb(240, 0, 240);");

    //Load default page
    if (address == "")      this->myWebView->load( QUrl(QString("http://%1").arg(DEFAULT_HOST_URL)) );
    else                    this->myWebView->load( QUrl(address) );
}

MainWindow::~MainWindow()
{
    isShuttingDown = true;
    delete this->mySocket;
    delete this->myWebView;
    delete this->myWebPage;
    delete this->ui;
}

void MainWindow::receiveArgs(const QString &argsString)
{
    QStringList argsList = argsString.split(ARGS_SPLIT_TOKEN);
    int argCount = argsList.count();
    if (argCount < 2)
        return;

    //QString execPath = argsList[0];
    QString command = argsList[1].toUpper();
    argsList.removeFirst();
    argsList.removeFirst();

    QByteArray string = processStatelessCommand(command.toLatin1(), argsList);
    if (string != UNIMPLEMENTED)            qDebug("%s: %s", TAG, string.constData());
    else                                    qDebug("Invalid argument");
}

void MainWindow::keyPressEvent ( QKeyEvent * event )
{
    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
    int keycode = event->key();
    bool autoRepeat = event->isAutoRepeat();

    //autoRepeat doesn't work with current IR driver anyway
    if (autoRepeat)
        return;

    switch (keycode)
    {
        case Qt::Key_Up:
            up = currentEpochMs;
            remoteControlKey("up");
            return;
        case Qt::Key_Down:
            down = currentEpochMs;
            remoteControlKey("down");
            return;
        case Qt::Key_Left:
            left = currentEpochMs;
            remoteControlKey("left");
            return;
        case Qt::Key_Right:
            right = currentEpochMs;
            remoteControlKey("right");
            return;

        case Qt::Key_Enter:
        case Qt::Key_Return:
            center = currentEpochMs;
            remoteControlKey("center");
            return;
        case Qt::Key_PageUp:
            cpanel = currentEpochMs;
            remoteControlKey("cpanel");
            return;
        case Qt::Key_PageDown:
            widget = currentEpochMs;
            remoteControlKey("widget");
            return;

        case Qt::Key_1:
            hidden1 = currentEpochMs;
            remoteControlKey("reset");
            return;
        case Qt::Key_2:
            hidden2 = currentEpochMs;
            remoteControlKey("reset");
            return;
    }

    //Default behavior
    QWidget::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent  ( QKeyEvent * event )
{
    static qint64 longClickThresholdMs1 = 3000;
    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
    int keycode = event->key();

    switch (keycode)
    {
        case Qt::Key_PageUp:
            if (cpanel >= 0 && currentEpochMs - cpanel > longClickThresholdMs1) {
                qDebug("%s: [keyboard override] long-press ControlPanel key (%lldms)", TAG, currentEpochMs-cpanel);
                remoteControlKey("reset");
            }
            cpanel = 0;
            return;

        case Qt::Key_PageDown:
            if (widget >= 0 && currentEpochMs - widget > longClickThresholdMs1) {
                qDebug("%s :[keyboard override] long-press Widget key (%lldms)", TAG, currentEpochMs-widget);
                remoteControlKey("reset");
            }
            widget = 0;
            return;
    }

    //Default behavior
    QWidget::keyReleaseEvent(event);
}
