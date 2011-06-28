#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>
#include <QProcess>
#include <QWebFrame>
#include <QUrl>
#include <QKeyEvent>

void MainWindow::sendSocketHello(SocketResponse *response)
{
    response->setCommand("Hello");
    response->setParameter("type", "netvbrowser");
    response->setParameter("version", "1.0");
    response->write();
}

QByteArray MainWindow::processStatelessCommand(QByteArray command, QStringList argsList)
{
    //command name
    if (command.length() < 0)
        return UNIMPLEMENTED;
    command = command.toUpper();

    //arguments
    int argCount = argsList.count();

    if (command == "MINIMIZE")
    {
        this->setVisible(false);
        return command;
    }

    else if (command == "MAXIMIZE" || command == "FULLSCREEN")
    {
        this->showFullScreen();
        return command;
    }

    else if (command == "SHOW")
    {
        this->showFullScreen();
        return command;
    }

    else if (command == "HIDE")
    {
        this->setVisible(false);
        return command;
    }

    else if (command == "REDRAW" || command == "UPDATE")
    {
        this->myWebView->update();
        return command;
    }

    else if (command == "QUIT" || command == "EXIT" || command == "TERMINATE")
    {
        QApplication::exit(0);
        return command;
    }

    else if (command == "RESTART")
    {
        //This will just ignore the 'singleton' class behaviour. Awesome!
        QProcess::startDetached( QApplication::applicationFilePath() );
        QApplication::exit(0);
        return command;
    }

    else if (command == "RESET")
    {
        resetWebview();
        return command;
    }

    else if (command == "BACKGROUNDTRANSPARENT_ON")
    {
        //Transparent background (the <body> tag must not contain 'bgcolor' property)
        QPalette palette = myWebView->palette();
        palette.setBrush(QPalette::Base, Qt::transparent);
        this->myWebView->page()->setPalette(palette);
        this->myWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);
        this->myWebView->update();
        printf("BackgroundTransparent: On \n");
        return command;
    }

    else if (command == "BACKGROUNDTRANSPARENT_OFF")
    {
        this->myWebView->page()->setPalette( myWebView->style()->standardPalette() );
        this->myWebView->update();
        printf("BackgroundTransparent: Off \n");
        return command;
    }

    else if (command == "INVERTCOLOR_ON")
    {
        this->myWebView->setInvertColor( true );
        this->myWebView->update();
        printf("InvertColor: On \n");
        return command;
    }

    else if (command == "INVERTCOLOR_OFF")
    {
        myWebView->setInvertColor( false );
        myWebView->update();
        printf("InvertColor: Off \n");
        return command;
    }

    //----------------------------------------------------

    else if ((command == "INVERTCOLOR" || command == "INVERTCOLOUR") && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";
        if (isOn)           return processStatelessCommand("INVERTCOLOR_ON");
        else                return processStatelessCommand("INVERTCOLOR_OFF");
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
        return command;
    }

    else if (command == "BACKGROUNDTRANSPARENT" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";
        if (isOn)              return processStatelessCommand("BACKGROUNDTRANSPARENT_ON");
        else                   return processStatelessCommand("BACKGROUNDTRANSPARENT_OFF");
    }

    else if (command == "SHOWHIDE" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isShow = param == "TRUE" || param == "YES" || param == "ON" || param == "SHOW";
        if (isShow)            return processStatelessCommand("SHOW");
        else                   return processStatelessCommand("HIDE");
    }

    else if (command == "SETURL" && argCount >= 1)
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
        return command;
    }

    else if (command == "SETHTML" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->setHtml(param, QUrl("http://localhost"));
        printf( "Setting new HTML content \n" );

        //Hide scrollbars
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
        return command;
    }

    else if (command == "REMOTECONTROL" && argCount >= 1)
    {
        QString param = argsList[0];
        //printf( "Remote control button: %s \n", param.toLatin1().constData() );

        if (param.toUpper() == "RESET")
        {
            resetWebview();
            return command;
        }

        QString javascriptString = QString("fButtonPress('%1');").arg(param);
        return (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
    }

    //----------------------------------------------------

    else if (command == "JAVASCRIPT" && argCount >= 1)
    {
        QString param = argsList.join(" ");
        printf( "Executing JavaScript: %s \n", param.toLatin1().constData() );
        return (this->myWebView->page()->mainFrame()->evaluateJavaScript(param)).toByteArray();
    }

    else if (command == "SETBOX" && argCount >= 4)
    {
        int x = argsList[0].toInt();
        int y = argsList[1].toInt();
        int w = argsList[2].toInt();
        int h = argsList[3].toInt();

        this->showNormal();
        this->setGeometry(x,y,w,h);
        return command;
    }

    else if (command == "KEY" && argCount >= 1)
    {
        QString param = argsList[0];
        bool convertOK = false;
        int keycode = param.toInt(&convertOK);
        if (!convertOK)
            return "invalid key";

        QKeyEvent key(QEvent::KeyPress, (Qt::Key)keycode, Qt::NoModifier);
        QApplication::sendEvent(this->myWebView, &key);

        if (key.isAccepted())
        {
            //everything is ok
        } else {
            //something wrong
        }
        return command;
    }

    return UNIMPLEMENTED;
}
