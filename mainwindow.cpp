#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QStringList>
#include <QWebFrame>
#include <QUrl>

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

    //Transparent background (page content dependent)
    QPalette palette = myWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    myWebView->page()->setPalette(palette);
    myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    //Default background color (white)
    this->setStyleSheet("background-color: rgb(255, 255, 255);");
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
        if (param.startsWith("www"))
            param = param.insert(0, "http://");
        QUrl newUrl(param, QUrl::TolerantMode);
        if (newUrl.isValid()) {
            myWebView->load( QUrl(param, QUrl::TolerantMode) );
            printf( "Setting new Url: %s \n", param.toLatin1().constData() );
        } else {
            printf( "Invalid Url  \n" );
        }
    }

    else if (command == "SetHtml" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->setHtml(param, QUrl("http://localhost"));
        printf( "Setting new HTML content \n" );
    }

    else if (command == "JavaScript" && argCount >= 1)
    {
        QString param = argsList.join(" ");
        myWebView->page()->mainFrame()->evaluateJavaScript(param);
        printf( "Executing JavaScript: %s \n", param.toLatin1().constData() );
    }

    else if (command == "InvertColor" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";
        myWebView->setInvertColor( isOn );
        myWebView->update();
        if (isOn)           printf("InvertColor: On \n");
        else                printf("InvertColor: Off \n");
    }

    else if (command == "BackgroundColor" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        if (param.startsWith("#") && param.length() == 7)
        {
            this->setStyleSheet( QString("background-color: %1;").arg(param) );
            this->update();
            printf("Setting new background color: %s \n", param.toLatin1().constData() );
        }
        else if (param.count(",") == 2 && param.length() <= 11)
        {
            this->setStyleSheet( QString("background-color: rgb(%1);").arg(param) );
            this->update();
            printf("Setting new background color: %s \n", param.toLatin1().constData() );
        }
        else
        {
            printf("Invalid argument. Example: '#0080FF' or '0,128,255' \n");
        }
    }

    else if (command == "BackgroundTransparent" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";

        if (isOn)
        {
            //Transparent background (the <body> tag must not contain 'bgcolor' property)
            QPalette palette = myWebView->palette();
            palette.setBrush(QPalette::Base, Qt::transparent);
            myWebView->page()->setPalette(palette);
            myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);
            printf("BackgroundTransparent: On \n");
        }
        else
        {
            myWebView->page()->setPalette( myWebView->style()->standardPalette() );
            printf("BackgroundTransparent: Off \n");
        }
        myWebView->update();
    }

    else
    {
        printf("Invalid arguments");
    }
}
