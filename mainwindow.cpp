#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QStringList>
#include <QProcess>
#include <QWebFrame>
#include <QUrl>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->isShuttingDown = false;
    this->cPanelLoaded = false;
    this->updateCPanel = false;
    this->myWebView = NULL;
    this->mySocket = NULL;
    this->port = DEFAULT_PORT;
    this->enNativeKeyboard = ENABLE_NATIVE_KB;
    this->enKeepAliveTimer = ENABLE_KEEPALIVE;

    //Multi-tab
    this->currentWebViewTab = 0;
    for (int i=0;i<MAX_TABS; i++)
        myWebViewArray[i] = NULL;

    //Keystroke handler
    keyStrokeTimer.setInterval(KEY_TIMEOUT);
    keyStrokeTimer.setSingleShot(true);
    QObject::connect(&keyStrokeTimer, SIGNAL(timeout()), this, SLOT(slot_keyStrokeTimeout()));

    //Focus input handler
    focusInputTimer.setInterval(FOCUS_INPUT_TIMEOUT);
    focusInputTimer.setSingleShot(false);
    QObject::connect(&focusInputTimer, SIGNAL(timeout()), this, SLOT(slot_updateFocusInput()));

    //Ping the page every 40 seconds
    keepAliveTimer.setInterval(KEEPALIVE_TIMEOUT);
    keepAliveTimer.setSingleShot(false);
    QObject::connect(&keepAliveTimer, SIGNAL(timeout()), this, SLOT(slot_keepAliveTimeout()));

    ui->setupUi(this);

    //Default background color for MainWindow (bright pink chroma key color)
    this->setStyleSheet("background-color: rgb(240, 0, 240);");

#ifdef ENABLE_QWS_STUFF
    //Chromeless window
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);     //Set window to fixed size
    this->setWindowFlags(Qt::CustomizeWindowHint);              //Set window with no title bar
    this->setWindowFlags(Qt::FramelessWindowHint);              //Set a frameless window
#endif

    initWebViewTab(DEFAULT_TAB);
    this->setupSocket();

    //Previously was doing an system upgrade, NeTVBrowser get killed
    if (FileExists(UPGRADE_PROGRESS_FILE))
    {
        //Continue reading the fifo
        this->setupUpgrade();

        //Here we have to reload the package list & recalculate the size
        this->resetWebViewTab(DEFAULT_TAB, QByteArray(UPDATE_PAGE) + "?continue=true");
        this->showWebViewTab(DEFAULT_TAB);
        return;
    }

    //Previously not doing an update
    this->resetWebViewTab(DEFAULT_TAB);
    this->showWebViewTab(DEFAULT_TAB);

    //Start pulling Control Panel from git
    //This will be done once socket to server is connected
    //requestUpdateCPanel();
}

MainWindow::~MainWindow()
{
    this->isShuttingDown = true;
    this->keyStrokeTimer.stop();
    this->keepAliveTimer.stop();
    if (this->mySocket != NULL)         delete this->mySocket;
    for (int i=0;i<MAX_TABS; i++)       if (myWebViewArray[i] != NULL)   {   delete myWebViewArray[i];      myWebViewArray[i] = NULL;   }
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

void MainWindow::paintEvent(QPaintEvent *pe)
{
    // normal painting
    QMainWindow::paintEvent(pe);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::NonCosmeticDefaultPen, false);
}

void MainWindow::resizeEvent ( QResizeEvent * event )
{
    QMainWindow::resizeEvent(event);

    //Fit all webviews
    int count = 0;
    for (int i=0; i<MAX_TABS; i++)
    {
        if (myWebViewArray[i] == NULL)
            continue;
        //Resize fullscreen
        myWebViewArray[i]->resize(this->frameGeometry().size());
        myWebViewArray[i]->move(0,0);
        count++;
    }

    qDebug("%s: resizeEvent (%d x %d) - resized %d tabs", TAG, this->frameGeometry().size().width(), this->frameGeometry().size().height(), count);
}
