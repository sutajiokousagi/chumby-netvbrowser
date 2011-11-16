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
    #include <QWSDisplay>
    #include <QTransformedScreen>
#endif


void MainWindow::sendSocketHello(SocketResponse *response)
{
    response->setCommand("Hello");
    response->setParameter("type", "netvbrowser");
    response->setParameter("version", "1.0");
    response->write();
}

void MainWindow::requestUpdateCPanel()
{
    qDebug("%s: request NeTVServer to update Control Panel from git repo", TAG);
    sendNeTVServerCommand("UpdateCPanel");
}

void MainWindow::requestSetDocroot(QByteArray newPath)
{
    qDebug("%s: request NeTVServer to change docroot to %s", TAG, newPath.constData());
    QMap<QByteArray, QByteArray> params;
    params.insert("value", newPath);
    sendNeTVServerCommand("SetDocroot", params);
}

void MainWindow::sendFocusedInput(QByteArray id, QByteArray value)
{
    QMap<QByteArray, QByteArray> params;
    params.insert("id", id);
    params.insert("value", value);
    sendNeTVServerCommand("TextInput", params);
}

void MainWindow::sendNeTVServerCommand(QByteArray command)
{
    QMap<QByteArray, QByteArray> params;
    sendNeTVServerCommand(command, params);
}

void MainWindow::sendNeTVServerCommand(QByteArray command, QMap<QByteArray, QByteArray> params)
{
    SocketResponse *response = new SocketResponse(this->mySocket);
    if (params.size() > 0)
    {
        QMapIterator<QByteArray, QByteArray> i(params);
        while (i.hasNext()) {
            i.next();
            response->setParameter(i.key(), i.value());
        }
    }
    response->setCommand(command);
    response->write();
}

void MainWindow::setURL(QString address)
{
    if (!address.startsWith("http://") && !address.startsWith("https://"))
        address = address.insert(0, "http://");

    QUrl newUrl(address, QUrl::TolerantMode);
    if (newUrl.isValid())   myWebView->load( QUrl(address, QUrl::TolerantMode) );
    else                    qDebug("%s: Invalid Url", TAG);

    //Hide scrollbars
    myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    myWebView->page()->mainFrame ()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
}

QByteArray MainWindow::remoteControlKey(bool isRepeat, QByteArray buttonName, int oneSecCount /* = 1 */)
{
    qDebug("%s: [keyboard override] %s (%d)", TAG, buttonName.constData(), oneSecCount);
    QString javascriptString = QString("fButtonPress('%1',%2,%3);").arg(QString(buttonName)).arg(oneSecCount).arg(QString(isRepeat?"true":"false"));
    return (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
}

void MainWindow::slot_keepAliveTimeout()
{
    //This is performed on DEFAULT_TAB only

    //If not initialized (should not happen)
    MyWebView * refWebView = myWebViewArray[DEFAULT_TAB];
    if (refWebView == NULL)
    {
        this->initWebViewTab(DEFAULT_TAB);
        this->resetAllTab();
        return;
    }

    //Stopping keep-alive timer
    if (this->enKeepAliveTimer == false) {
        this->keepAliveTimer.stop();
        return;
    }

    //We don't need to check if fCheckAlive exist or not. It will failed anyway.
    QString isAlive = refWebView->page()->mainFrame()->evaluateJavaScript( QString("fCheckAlive();") ).toString();
    QString url = refWebView->page()->mainFrame()->url().toString();

    if (isAlive.toLower() == "true")        qDebug("%s: [keep alive] [OK] %s", TAG, url.toLatin1().constData());
    else                                    qDebug("%s: [keep alive] [FAILED] %s", TAG, url.toLatin1().constData());

    //Update text input focus if still alive
    if (isAlive.toLower() == "true")
    {
        this->updateFocusedInputScreenshot();
        return;
    }

    if (url.contains(DEFAULT_HOST_URL))     refWebView->reload();
    else                                    this->resetWebViewTab(DEFAULT_TAB);
}

void MainWindow::slot_requestUpdateCPanel()
{
    this->requestUpdateCPanel();
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
        resetWebViewTab(DEFAULT_TAB);
        this->showWebViewTab(DEFAULT_TAB);
        return command;
    }

    else if (command == "KEEPALIVE" && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        bool isOn = param == "TRUE" || param == "YES" || param == "ON";
        if (isOn)     {      if (!keepAliveTimer.isActive())    keepAliveTimer.start();     this->enKeepAliveTimer = true;      }
        else          {      if (keepAliveTimer.isActive())     keepAliveTimer.stop();      this->enKeepAliveTimer = false;     }
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
        qDebug("BackgroundTransparent: On \n");
        return command;
    }

    else if (command == "BACKGROUNDTRANSPARENT_OFF")
    {
        this->myWebView->page()->setPalette( myWebView->style()->standardPalette() );
        this->myWebView->update();
        qDebug("BackgroundTransparent: Off \n");
        return command;
    }

    else if (command == "INVERTCOLOR_ON")
    {
        this->myWebView->setInvertColor( true );
        this->myWebView->update();
        qDebug("InvertColor: On \n");
        return command;
    }

    else if (command == "INVERTCOLOR_OFF")
    {
        myWebView->setInvertColor( false );
        myWebView->update();
        qDebug("InvertColor: Off \n");
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

    //----------------------------------------------------

    else if (command == "SETDOCROOT")
    {
        qDebug("%s: --------------------------------------------------------------", TAG);
        qDebug("%s: docroot changed to %s. Reload now!", TAG, qPrintable(argsList[0]));
        qDebug("%s: --------------------------------------------------------------", TAG);
        cPanelLoaded = false;
        initWebViewTab(DEFAULT_TAB);
        resetWebViewTab(DEFAULT_TAB);
        showWebViewTab(DEFAULT_TAB);
        return command;
    }

    else if (command == "SETURL" && argCount >= 1)
    {
        QString param = argsList[0];
        if (!param.startsWith("http://") && !param.startsWith("https://"))
            param = param.insert(0, "http://");

        setURL(param);

        if (param != "")
            return QString("%1 %2").arg(command.constData()).arg(param).toLatin1();
        return command;
    }

    else if ((command == "TAB" || command == "MULTITAB") && argCount >= 1)
    {
        QString tabIndex = argsList[0];
        QString action = (argCount >= 2) ? argsList[1] : "";
        QString param = (argCount >= 3) ? argsList[2] : "";
        if (argCount == 1)
        {
            if (action.toUpper() == "DESTROYALL")     {      hideOtherWebViewTab(DEFAULT_TAB, true);       showWebViewTab(DEFAULT_TAB);               }
            else if (action.toUpper() == "HIDEALL")   {      hideOtherWebViewTab(DEFAULT_TAB, true);       showWebViewTab(DEFAULT_TAB);               }
        }
        else if (argCount >= 3)
        {
            int index = tabIndex.toInt();
            if (action.toUpper() == "LOAD")           {      loadWebViewTab(index, param.toLatin1());                   }
            else if (action.toUpper() == "SHOW")      {      loadWebViewTab(index, param.toLatin1());                   }
        }
        else if (argCount >= 2)
        {
            int index = tabIndex.toInt();
            if (action.toUpper() == "HIDE")           {      hideOtherWebViewTab(DEFAULT_TAB, false);      showWebViewTab(DEFAULT_TAB);               }
            else if (action.toUpper() == "DESTROY")   {      hideWebViewTab(index, true);                  showWebViewTab(DEFAULT_TAB);               }
            else if (action.toUpper() == "SHOW")      {      showWebViewTab(index);                                                                   }
            else                                             loadWebViewTab(index, action.toLatin1());
        }
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

    //----------------------------------------------------

    else if (command == "REMOTECONTROL" && argCount >= 1)
    {
        QString param = argsList[0];
        Qt::Key keycode = getKeyCodeFromName(param);
        if (keycode == Qt::Key_unknown)
            return QString("%1 %2").arg(command.constData()).arg("Unknown key").toLatin1();

        //Trigger native keyboard event
        triggerKeycode(keycode);

        return QString("%1 %2").arg(command.constData()).arg(param.toLatin1().constData()).toLatin1();
    }

    else if (command == "KEY" && argCount >= 1)
    {
        QString param = argsList[0];
        Qt::Key keycode = getKeyCodeFromName(param);
        if (keycode == Qt::Key_unknown)
            return QString("%1 %2").arg(command.constData()).arg("Unknown key").toLatin1();

        //Trigger native keyboard event
        triggerKeycode(keycode);

        return QString("%1 %2").arg(command.constData()).arg(param.toLatin1().constData()).toLatin1();
    }

    else if ((command == "NATIVEKB" || command == "NATIVEKEYBOARD" || command == "KEYBOARD") && argCount >= 1)
    {
        QString param = argsList[0].toUpper();
        enNativeKeyboard = param == "TRUE" || param == "YES" || param == "ON";

        //Ignore mouse & keyboard
        if (!enNativeKeyboard)                          myWebViewArray[DEFAULT_TAB]->setEnabled(false);
        else if (myWebViewArray[DEFAULT_TAB] != NULL)   myWebViewArray[DEFAULT_TAB]->setEnabled(true);

        return QString("%1 %2").arg(command.constData()).arg(param.toLatin1().constData()).toLatin1();
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
        return command;
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
        this->setGeometry(0,0,w,h);
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
        this->setGeometry(0,0,w,h);
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

    else if (command == "ROTATE" && argCount >= 1)
    {
        if (QScreen::instance()->classId() != QScreen::TransformedClass)
            return QByteArray("The screen driver is not a QScreen::TransformedClass subclass");

        QString eventData = argsList[0];
        if (eventData.toUpper() == "90")
            QWSDisplay::instance()->setTransformation(1);
        else if (eventData.toUpper() == "180")
            QWSDisplay::instance()->setTransformation(2);
        else if (eventData.toUpper() == "270")
            QWSDisplay::instance()->setTransformation(3);
        else
            QWSDisplay::instance()->setTransformation(0);
        QWSServer::instance()->refresh();
        return command;
    }

#endif

    //----------------------------------------------------
    // opkg upgrade commands

    else if (command == "UPDATEPROGRESS")
    {
        QString percentage = argsList[0];
        QString pkgname = argsList[1];
        QString sizeprogress = "0";  //argsList[2];
        quint64 pkgsize = getPackageSize(pkgname.toLatin1());       // = 0 if pkgname is not in list-upgradable

        //Notify JavaScript
        QString eventData = QString("<percentage>%1</percentage><pkgname>%2</pkgname><pkgsize>%3</pkgsize><sizeprogress>%4</sizeprogress>").arg(percentage).arg(pkgname).arg(pkgsize).arg(sizeprogress);
        QString javascriptString = QString("fUPDATEEvents('progress', '%1');").arg(QString(QUrl::toPercentEncoding(eventData)));
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        return javaResult;
    }

    else if (command == "CONFIGUREPROGRESS" && argCount >= 2)
    {
        QString percentage = argsList[0];
        QString pkgname = argsList[1];

        //Notify JavaScript
        QString eventData = QString("<percentage>%1</percentage><pkgname>%2</pkgname>").arg(percentage).arg(pkgname);
        QString javascriptString = QString("fUPDATEEvents('configuring', '%1');").arg(QString(QUrl::toPercentEncoding(eventData)));
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        return javaResult;
    }

    else if (command == "UPDATEREADY" && argCount >= 2)
    {
        QString numpkg = argsList[0];
        QString reboot = argsList[1];

        setupUpgrade();
        resetUpgrade();
        getUpgradablePackageList();
        getDownloadedPackageSize();
        doUpgrade(reboot == "1" ? true : false);

        //Notify JavaScript (this implementation doesn't work sometimes)
        //If customer choose to replace with their own UI, this implementation will fail
        QString javascriptString = QString("fUPDATEEvents('starting', '%1');").arg(argsList[1]);
        QByteArray javaResult = (this->myWebView->page()->mainFrame()->evaluateJavaScript(javascriptString)).toByteArray();
        if (javaResult == "ok")
            return javaResult;

        //JavaScript failed to execute, force NeTVBrowser to switch to Update UI
        setURL(UPDATE_PAGE);
        return "ok";
    }

    else if (command == "UPGRADECOMPLETE" || command == "UPGRADECOMP")
    {
        return command;
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

QByteArray MainWindow::getIRKeyName(int keycode)
{
    switch (keycode)
    {
        case Qt::Key_PageUp:        return "cpanel";
        case Qt::Key_PageDown:      return "widget";
        case Qt::Key_Up:            return "up";
        case Qt::Key_Down:          return "down";
        case Qt::Key_Left:          return "left";
        case Qt::Key_Right:         return "right";
        case Qt::Key_Return:        return "center";
        case Qt::Key_Enter:         return "center";
        default:                    return "";
    }
    return "";
}

Qt::Key MainWindow::getKeyCodeFromName(QString keyname)
{
    keyname = keyname.toLower();
    if (keyname.length() < 1)
        return Qt::Key_unknown;

    if (keyname == "left")              return Qt::Key_Left;
    else if (keyname == "right")        return Qt::Key_Right;
    else if (keyname == "up")           return Qt::Key_Up;
    else if (keyname == "down")         return Qt::Key_Down;
    else if (keyname == "enter")        return Qt::Key_Return;
    else if (keyname == "center")       return Qt::Key_Return;
    else if (keyname == "\n")           return Qt::Key_Return;
    else if (keyname == "cpanel")       return Qt::Key_PageUp;
    else if (keyname == "widget")       return Qt::Key_PageDown;
    else if (keyname == "pgup")         return Qt::Key_PageUp;
    else if (keyname == "pgdown")       return Qt::Key_PageDown;
    else if (keyname == "pageup")       return Qt::Key_PageUp;
    else if (keyname == "pagedown")     return Qt::Key_PageDown;

    else if (keyname == "esc")          return Qt::Key_Escape;
    else if (keyname == "del")          return Qt::Key_Delete;
    else if (keyname == "delete")       return Qt::Key_Delete;
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
