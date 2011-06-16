#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QStringList>
#include <QWebFrame>
#include <QUrl>
#include <QProcess>

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

    //Hide scrollbars
    myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Transparent background (page content dependent)
    QPalette palette = myWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    myWebView->page()->setPalette(palette);
    myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    //Default background color (white)
    this->setStyleSheet("background-color: rgb(255, 255, 255);");

    //Chromeless window
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);     //Set window to fixed size
    this->setWindowFlags(Qt::CustomizeWindowHint);              //Set window with no title bar
    this->setWindowFlags(Qt::FramelessWindowHint);              //Set a frameless window
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
    QString command = argsList[1].toUpper();;
    argsList.removeFirst();
    argsList.removeFirst();
    argCount = argsList.count();

    if (command == "SETURL" && argCount >= 1)
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

        //Hide scrollbars
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    }

    else if (command == "SETHTML" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->setHtml(param, QUrl("http://localhost"));
        printf( "Setting new HTML content \n" );

        //Hide scrollbars
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    }

    else if (command == "JAVASCRIPT" && argCount >= 1)
    {
        QString param = argsList.join(" ");
        myWebView->page()->mainFrame()->evaluateJavaScript(param);
        printf( "Executing JavaScript: %s \n", param.toLatin1().constData() );
    }

    else if ((command == "INVERTCOLOR" || command == "INVERTCOLOUR") && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";
        myWebView->setInvertColor( isOn );
        myWebView->update();
        if (isOn)           printf("InvertColor: On \n");
        else                printf("InvertColor: Off \n");
    }

    else if ((command == "BACKGROUNDCOLOR" || command == "BACKGROUNDCOLOUR") && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        if (param.startsWith("#") && param.length() == 7)
        {
            this->setStyleSheet( QString("background-color: %1;").arg(param) );
            this->update();
            printf("Setting new background color: %s \n", param.toLatin1().constData() );
        }
        else if (param.count(",") == 2 && param.length() <= 11 && param.length() >= 5)
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

    else if (command == "BACKGROUNDTRANSPARENT" && argCount >= 1)
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

    else if (command == "SHOWHIDE" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isShow = param == "TRUE" || param == "YES" || param == "ON" || param == "SHOW";

        if (isShow)            this->showFullScreen();
        else                   this->setVisible(false);
    }

    else if (command == "MINIMIZE")
    {
        this->setVisible(false);
    }

    else if (command == "MAXIMIZE" || command == "FULLSCREEN")
    {
        this->showFullScreen();
    }

    else if (command == "SETBOX" && argCount >= 4)
    {
        int x = argsList[0].toInt();
        int y = argsList[1].toInt();
        int w = argsList[2].toInt();
        int h = argsList[3].toInt();

        this->showNormal();
        this->setGeometry(x,y,w,h);
    }

    else if (command == "QUIT" || command == "EXIT" || command == "TERMINATE")
    {
        QApplication::exit(0);
    }

    else if (command == "RESTART")
    {
        //This will just override the 'singleton' class. Awesome!
        QProcess::startDetached( QApplication::applicationFilePath() );
        QApplication::exit(0);
    }

    else
    {
        printf("Invalid arguments");
    }
}
