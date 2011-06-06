#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStringList>
#include <QWebFrame>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Remove the existing webview
    this->ui->rootLayout->removeWidget( this->ui->webView );

    //Replace it with our custom webview
    myWebView = new MyWebView(this);
    this->ui->rootLayout->addWidget(myWebView);

    //Do any other customization on default view state
    myWebView->setInvertColor(false);
    myWebView->load( QUrl(DEFAULT_URL) );
}

MainWindow::~MainWindow()
{
    delete myWebView;
    delete ui;
}

void MainWindow::receiveArgs(const QString &argsString)
{
    QStringList argsList = argsString.split(ARGS_SPLIT_TOKEN);
    int argCount = argsList.count();
    if (argCount < 2)
        return;

    QString execPath = argsList[0];
    QString command = argsList[1];
    argsList.removeFirst();
    argsList.removeFirst();
    argCount = argsList.count();

    if (command == "SetUrl" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->load( QUrl(param) );
    }

    else if (command == "SetHtml" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->setHtml(param, QUrl("http://localhost"));
    }

    else if (command == "Javascript" && argCount >= 1)
    {
        QString param = argsList.join(" ");
        myWebView->page()->mainFrame()->evaluateJavaScript(param);
    }

    else if (command == "InvertColor" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        myWebView->setInvertColor( param == "TRUE" || param == "YES" );
    }

    //Transparent background (page content dependent)
    /*
    QPalette palette = myWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    myWebView->page()->setPalette(palette);
    myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    */
}
