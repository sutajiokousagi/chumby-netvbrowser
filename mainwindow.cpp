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
    isShuttingDown = false;
    this->myWebView = NULL;
    this->myWebPage = NULL;
    this->mySocket = NULL;
    this->port = DEFAULT_PORT;

    ui->setupUi(this);

    //Chromeless window
#ifdef Q_WS_QWS
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);     //Set window to fixed size
    this->setWindowFlags(Qt::CustomizeWindowHint);              //Set window with no title bar
    this->setWindowFlags(Qt::FramelessWindowHint);              //Set a frameless window
#endif

    this->setupWebview();
    this->setupSocket();
}

void MainWindow::setupWebview()
{
    //Remove the existing webview
    this->ui->rootLayout->removeWidget( this->ui->webView );

    //Replace it with our custom webview
    this->myWebView = new MyWebView(this);
    this->myWebPage = new MyWebPage(this);
    this->ui->rootLayout->addWidget(this->myWebView);

    resetWebview();
}

void MainWindow::resetWebview()
{
    qDebug("NeTVBrowser:resetWebview");

    //Do any other customization on default view state
    this->myWebView->setInvertColor(false);
    this->myWebView->setPage(myWebPage);
    this->myWebView->load( QUrl(QString("http://%1").arg(DEFAULT_HOST_URL)) );


    //Hide scrollbars
    this->myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    this->myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Transparent background (page content dependent)
    QPalette palette = this->myWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    this->myWebView->page()->setPalette(palette);
    this->myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    //Default background color (bright pink chroma key color)
    this->setStyleSheet("background-color: rgb(240, 0, 240);");
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
    if (string != UNIMPLEMENTED)            qDebug("NeTVBrowser: %s", string.constData());
    else                                    qDebug("Invalid argument");
}

