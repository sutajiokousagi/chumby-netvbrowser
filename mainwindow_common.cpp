#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>
#include <QProcess>
#include <QWebFrame>
#include <QUrl>
#include <QKeyEvent>

#ifdef ENABLE_QWS_STUFF
    #include <QWSServer>
    #include <QScreen>
#endif


void MainWindow::sendSocketHello(SocketResponse *response)
{
    response->setCommand("Hello");
    response->setParameter("type", "netvbrowser");
    response->setParameter("version", "1.0");
    response->write();
}

QByteArray MainWindow::remoteControlKey(QByteArray buttonName)
{
    if (buttonName.toUpper() == "RESET")
    {
        resetWebview();
        return "";
    }
    qDebug("%s: [keyboard override] %s", TAG, buttonName.constData());
    QString javascriptString = QString("fButtonPress('%1');").arg(QString(buttonName));
    return (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
}

QByteArray MainWindow::processStatelessCommand(QByteArray command, QStringList argsList)
{
    //command name
    if (command.length() < 0)
        return UNIMPLEMENTED;
    command = command.toUpper();

    //arguments
    int argCount = argsList.count();

    if (command == "HELLO")
    {
        //Ignore this message
        return command;
    }

    else if (command == "MINIMIZE" || command == "HIDE")
    {
        this->setVisible(false);
        return command;
    }

    else if (command == "MAXIMIZE" || command == "FULLSCREEN" || command == "SHOW")
    {
        this->showFullScreen();
        return command;
    }

    else if (command == "RESET")
    {
        resetWebview();
        return command;
    }

    //----------------------------------------------------

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
        }
        else if (param.count(",") == 2 && param.length() <= 11 && param.length() >= 5)
        {
            this->setStyleSheet( QString("background-color: rgb(%1);").arg(param) );
            this->update();
        }
        else
        {
            printf("Invalid argument. Example: '#0080FF' or '0,128,255' \n");
        }
        return command;     //QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
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
        if (newUrl.isValid())   myWebView->load( QUrl(param, QUrl::TolerantMode) );
        else                    printf( "Invalid Url  \n" );

        //Hide scrollbars
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    else if (command == "SETHTML" && argCount >= 1)
    {
        QString param = argsList[0];
        myWebView->setHtml(param, QUrl("http://localhost"));

        //Hide scrollbars
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
        return command;
    }

    else if (command == "REMOTECONTROL" && argCount >= 1)
    {
        QString param = argsList[0];
        QByteArray javaResult = remoteControlKey(param.toLatin1());
        if (javaResult != "")
            return QString("%1 %2").arg(command.constData()).arg(javaResult.constData()).toLatin1();
        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    //----------------------------------------------------

    else if (command == "NMSTATECHANGED" && argCount >= 1)
    {
        QString param = argsList[0];

        //See http://projects.gnome.org/NetworkManager/developers/spec.html#type-NM_STATE
        if (param.toUpper() == "0")         param = "unknown";
        else if (param.toUpper() == "1")    param = "sleeping";
        else if (param.toUpper() == "2")    param = "connecting";
        else if (param.toUpper() == "3")    param = "connected";
        else if (param.toUpper() == "4")    param = "disconnected";

        QString javascriptString = QString("fNMStateChanged('%1');").arg(param);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult != "")
            return QString("%1 %2").arg(command.constData()).arg(javaResult.constData()).toLatin1();
        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    else if (command == "NMPROPERTIESCHANGED" && argCount >= 1)
    {
        //QString param = argsList[0];

        //To be decided

        return command;
    }

    else if (command == "NMDEVICEADDED" && argCount >= 1)
    {
        QString param = argsList[0];        //object path?

        QString javascriptString = QString("fNMDeviceAdded('%1');").arg(param);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult != "")
            return QString("%1 %2").arg(command.constData()).arg(javaResult.constData()).toLatin1();
        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    else if (command == "NMDEVICEREMOVED" && argCount >= 1)
    {
        QString param = argsList[0];        //object path?

        QString javascriptString = QString("fNMDeviceRemoved('%1');").arg(param);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult != "")
            return QString("%1 %2").arg(command.constData()).arg(javaResult.constData()).toLatin1();
        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    //----------------------------------------------------

    else if (command == "JAVASCRIPT" && argCount >= 1)
    {
        QString param = argsList.join(" ");
        //printf( "Executing JavaScript: %s \n", param.toLatin1().constData() );
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(param)).toByteArray();
        if (javaResult != "")
            return javaResult;
        return command;     //param.toLatin1();
    }

    else if (command == "SETBOX" && argCount >= 4)
    {
        int x = argsList[0].toInt();
        int y = argsList[1].toInt();
        int w = argsList[2].toInt();
        int h = argsList[3].toInt();

        this->showNormal();
        this->setGeometry(x,y,w,h);
        return command;     //QString("%1 %2 %3 %4 %5").arg(command.constData()).arg(x).arg(y).arg(w).arg(h).toLatin1();
    }

    else if (command == "KEY" && argCount >= 1)
    {
        QString param = argsList[0];
        Qt::Key keycode = getKeyCode(param);
        if (keycode == Qt::Key_unknown)
            return QString("%1 %2").arg(command.constData()).arg("Unknown key").toLatin1();

        QKeyEvent key1(QEvent::KeyPress, keycode, Qt::ShiftModifier);
        QApplication::sendEvent(this->myWebView, &key1);

        QKeyEvent key2(QEvent::KeyRelease, keycode, Qt::NoModifier);
        QApplication::sendEvent(this->myWebView, &key2);

        /*
        if (key.isAccepted())
        {
            qDebug("Key accepted");
        } else {
            qDebug("Key NOT accepted");
        }
        */
        return QString("%1 %2").arg(command.constData()).arg("Unknown key").toLatin1();
    }

    //----------------------------------------------------

#ifdef ENABLE_QWS_STUFF
    else if (command == "REFRESH" || command == "REDRAW")
    {
        //redraw the entire screen
        QWSServer::instance()->refresh();
    }
    else if (command == "SETRESOLUTION" && argCount >= 3)
    {
        int w = argsList[0].toInt();
        int h = argsList[1].toInt();
        int depth = argsList[2].toInt();
        QScreen::instance()->setMode(w,h,depth);
        this->showFullScreen();
        QWSServer::instance()->refresh();
        return QString("%1 %2 %3 %4").arg(command.constData()).arg(w).arg(h).arg(depth).toLatin1();
    }
    else if (command == "SETRESOLUTION" && argCount >= 1)
    {
        QStringList argsLs = argsList[0].split(",");
        if (argsList.count() < 3)
            return UNIMPLEMENTED;

        int w = argsLs[0].toInt();
        int h = argsLs[1].toInt();
        int depth = argsLs[2].toInt();
        QScreen::instance()->setMode(w,h,depth);
        this->showFullScreen();
        QWSServer::instance()->refresh();
        return QString("%1 %2 %3 %4").arg(command.constData()).arg(w).arg(h).arg(depth).toLatin1();

    }
    else if (command == "HDMI" && argCount >= 1)
    {
        QString eventName = argsList[0];
        QString javascriptString = QString("fHDMIEvent('%1');").arg(QString(eventName));
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult != "")
            return javaResult;
        return command;
    }
#endif

    //----------------------------------------------------
    // Update command

    else if (command == "UPDATEREADY" && argCount >= 2)
    {
        setupUpgrade();
        resetUpgrade();
        getUpgradablePackageList();
        getDownloadedPackageSize();
        doUpgrade();

        //Notify JavaScript
        QString javascriptString = QString("fUPDATEEvents('starting', '%1');").arg(argsList[1]);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        return javaResult;
    }

    else if (command == "UPDATEDONE" || command == "UPGRADEDONE")
    {
        upgradeDone();
        return command;
    }

    //----------------------------------------------------
    // Generic command
    // Directly passed to JavaScript ControlPanel as fCOMMANDEvent('arg1','arg2','arg3'...);

    else
    {
        QString paramString = "";
        for (int i=0; i<argCount; i++)
        {
            if (i < argCount-1)     paramString.append( QString("'%1',").arg(argsList[i]) );
            else                    paramString.append( QString("'%1'").arg(argsList[i]) );
        }
        QString javascriptString = QString("f%1Event(%2);").arg(QString(command)).arg(paramString);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult != "")
            return javaResult;
        return QByteArray(UNIMPLEMENTED) + ":" + command;
    }

    return QByteArray(UNIMPLEMENTED) + ":" + command;
}

Qt::Key MainWindow::getKeyCode(QString keyname)
{
    keyname = keyname.toLower();
    if (keyname.length() < 1)
        return Qt::Key_unknown;

    if (keyname == "left")              return Qt::Key_Left;
    else if (keyname == "right")        return Qt::Key_Right;
    else if (keyname == "up")           return Qt::Key_Up;
    else if (keyname == "down")         return Qt::Key_Down;
    else if (keyname == "enter")        return Qt::Key_Enter;
    else if (keyname == "center")       return Qt::Key_Enter;
    else if (keyname == "\n")           return Qt::Key_Enter;
    else if (keyname == "esc")          return Qt::Key_Escape;
    else if (keyname == "del")          return Qt::Key_Delete;
    else if (keyname == "backspace")    return Qt::Key_Backspace;
    else if (keyname == "space")        return Qt::Key_Space;
    else if (keyname == " ")            return Qt::Key_Space;
    else if (keyname == "+")            return Qt::Key_Plus;
    else if (keyname == "-")            return Qt::Key_Minus;
    else if (keyname == "*")            return Qt::Key_Asterisk;
    else if (keyname == "/")            return Qt::Key_Slash;
    else if (keyname == ".")            return Qt::Key_Period;
    else if (keyname == ",")            return Qt::Key_Comma;
    else if (keyname == "?")            return Qt::Key_Question;
    else if (keyname == "<")            return Qt::Key_Less;
    else if (keyname == ">")            return Qt::Key_Greater;
    else if (keyname == "=")            return Qt::Key_Equal;
    else if (keyname == "@")            return Qt::Key_At;
    else if (keyname == "!")            return Qt::Key_Exclam;
    else if (keyname == "%")            return Qt::Key_Percent;
    else if (keyname == "$")            return Qt::Key_Dollar;
    else if (keyname == ":")            return Qt::Key_Colon;
    else if (keyname == ";")            return Qt::Key_Semicolon;
    else if (keyname == "volup")        return Qt::Key_VolumeUp;
    else if (keyname == "voldown")      return Qt::Key_VolumeDown;
    else if (keyname == "mute")         return Qt::Key_VolumeMute;
    else if (keyname == "search")       return Qt::Key_Search;
    else if (keyname == "menu")         return Qt::Key_Menu;

    else if (keyname.length() >= 2)
    {
        if (keyname.startsWith('f'))
        {
            bool convertOK = false;
            keyname = keyname.right(keyname.length()-1);
            int findex = keyname.toInt(&convertOK);
            if (convertOK)
                return (Qt::Key)(Qt::Key_F1 + (findex-1));
        }
    }
    else
    {
        //a-z
        if (keyname[0].unicode() >= 'a' && keyname[0].unicode() <= 'z')
            return (Qt::Key)(Qt::Key_A + keyname[0].unicode() - 'a');

        if (keyname[0].unicode() >= '0' && keyname[0].unicode() <= '9')
            return (Qt::Key)(Qt::Key_0 + keyname[0].unicode() - 'a');
    }

    return Qt::Key_unknown;
}
