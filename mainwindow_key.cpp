#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QWebElementCollection>

void MainWindow::keyPressEvent ( QKeyEvent * event )
{
    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
    int keycode = event->key();
    bool isRepeat = event->isAutoRepeat();
    QByteArray keyName = getIRKeyName(keycode);

    if (!isRepeat)
        keyPressEpochMap.insert(keycode, currentEpochMs);

    //Special key - Event will be delivered 2 seconds later
    if (keycode == Qt::Key_HomePage)
    {
        if (isRepeat)
            return;
        addKeyStrokeHistory("setup");
        return;
    }

    //Default behavior
    if (keyName.length() <= 0) {
        qDebug("%s: keyPressEvent '%d'", TAG, keycode);
        QMainWindow::keyPressEvent(event);
        return;
    }

    //Detect combo sequence
    if (!isRepeat)
    {
        bool isCombo = addKeyStrokeHistory(keyName);
        if (!enNativeKeyboard && !isCombo)
            remoteControlKey(false, keyName);
    }
    else if (!enNativeKeyboard)
    {
        if (keycode != Qt::Key_PageUp && keycode != Qt::Key_PageDown)
            remoteControlKey(true, keyName);
    }

    remoteControlPageInteraction(keycode);
    if (enNativeKeyboard)
        QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent  ( QKeyEvent * event )
{
    //Detect long-pressed of ControlPanel key

    static qint64 longClickThresholdMs1 = 3000;
    int keycode = event->key();

    if (keycode == Qt::Key_PageUp)
    {
        qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
        qint64 lastEpochMs = keyPressEpochMap.value(keycode);
        qint64 deltaEpochMs = currentEpochMs - lastEpochMs;

        if (lastEpochMs >= 0 && deltaEpochMs > longClickThresholdMs1 && deltaEpochMs < 20000)
        {
            qDebug("%s: [keyboard override] long-press ControlPanel key (%lldms)", TAG, deltaEpochMs);
            resetAllTab();
            return;
        }
    }

    //Force update screenshot of current focused text input (if any)
    this->updateFocusedInputScreenshot();

    //Default behavior
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::triggerKeycode(int keycode, int count /* = 1 */)
{
    for (int i=0; i < count; i++)
    {
        QKeyEvent keyPressEvent(QEvent::KeyPress, keycode, Qt::NoModifier);
        sendWebViewTabEvent(this->currentWebViewTab, &keyPressEvent);

        QKeyEvent keyReleaseEvent(QEvent::KeyRelease, keycode, Qt::NoModifier);
        sendWebViewTabEvent(this->currentWebViewTab, &keyReleaseEvent);
    }
}

bool MainWindow::addKeyStrokeHistory(QString keyName)
{
    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
    keyStrokeHistory.prepend( QString("%1|%2").arg(QString(keyName)).arg(currentEpochMs) );
    while (!keyStrokeHistory.isEmpty() && keyStrokeHistory.size() > 16)
        keyStrokeHistory.removeLast();

    //Delayed action for SETUP button only
    if (keyName.toUpper() == "SETUP" && !keyStrokeTimer.isActive())
    {
        keyStrokeTimer.start();
        keyStrokeTimerEpoch = currentEpochMs;
    }

    //Detect complex pattern
    if (keyStrokeHistory.size() >= 9)
    {
        QString keyString;
        bool hit = false;
        for (int i=0; i < 9; i++)
            keyString = QString(keyStrokeHistory.at(i).split("|").at(0)).append(QString(",")).append(keyString);

        if (keyString.contains("up,right,down,left,up,right,down,left,center"))
        {
            qDebug("%s: key combo: %s", TAG, keyString.toLatin1().constData());
            qDebug("%s: starting Access Point mode for Factory...", TAG);
            sendNeTVServerCommand("STARTAPFACTORY");
            setURL(FACTORY_PAGE);
            hit = true;
        }
        else if (keyString.contains("up,right,down,left,up,right,down,left,cpanel")) {
            qDebug("%s: key combo: %s", TAG, keyString.toLatin1().constData());
            qDebug("%s: do something cool 1", TAG);
            hit = true;
        }
        else if (keyString.contains("up,right,down,left,up,right,down,left,widget")) {
            qDebug("%s: key combo: %s", TAG, keyString.toLatin1().constData());
            qDebug("%s: do something cool 2", TAG);
            hit = true;
        }
        if (hit)
            keyStrokeHistory.clear();
        return hit;
    }
    return false;
}

void MainWindow::slot_keyStrokeTimeout()
{
    //This seems to make the browser hang
    //keyStrokeTimer.stop();

    //Get the keys pressed within 1.5 seconds
    QStringList tempOneSecList;
    for (int i=0; i < keyStrokeHistory.size(); i++)
    {
        QString temp = keyStrokeHistory.at(i);
        bool ok = false;
        if (temp.split("|").size() < 2)
            continue;
        qint64 time = temp.split("|").at(1).toLongLong(&ok);
        if (!ok)
            continue;
        if (time < keyStrokeTimerEpoch)
            continue;
        tempOneSecList.prepend(temp);
    }

    //Counting which keys are pressed within 1.5 second
    QMap<QString, int> keyCountMap;
    for (int i=0; i < tempOneSecList.size(); i++)
    {
        QString temp = tempOneSecList.at(i);
        QString key = temp.split("|").at(0);
        if (!keyCountMap.contains(key)) {
            keyCountMap.insert(key, 1);
            continue;
        }
        int count = keyCountMap.value(key);
        count++;
        keyCountMap.insert(key, count);
    }

    //Detect simple counting pattern
    QMapIterator<QString, int> i(keyCountMap);
    while (i.hasNext())
    {
        i.next();
        QString key = i.key();
        int count = i.value();
        if (key.toUpper() == "SETUP")
        {
            if (count > 0)
            {
                //Call a system script to force rekey & hotplug on FPGA
                this->Execute("/usr/bin/fpga_setup", QStringList() << QString().setNum(count));

                //Deliver event to Control Panel
                remoteControlKey(false, key.toLatin1(), count);
            }
        }
        else if (count > 1)
            remoteControlKey(false, key.toLatin1(), count);
    }

    tempOneSecList.clear();
    keyCountMap.clear();
}

void MainWindow::remoteControlPageInteraction(int keycode)
{
    if (isWebViewTabVisible(SECOND_TAB))
    {
        QSize frameSize = this->frameGeometry().size();
        if (keycode == Qt::Key_Up)             scrollWebViewTabDelta(SECOND_TAB, 0, -frameSize.height() / 10);      //magic number 10
        else if (keycode == Qt::Key_Down)      scrollWebViewTabDelta(SECOND_TAB, 0, frameSize.height() / 10);       //magic number 10
    }
}

//---------------------------------------------------------------------------
// Trigger input event on Android/iOS
//---------------------------------------------------------------------------

void MainWindow::slot_updateFocusInput()
{
    updateFocusedInputScreenshot();
}

QWebElement MainWindow::getFocusedInputElement()
{
    if (this->myWebView == NULL || this->myWebView->page() == NULL || this->myWebView->page()->currentFrame() == NULL)
        return QWebElement();

    QWebFrame* currentFrame = this->myWebView->page()->currentFrame();
    QWebElementCollection collection = currentFrame->findAllElements("input[type=text]");
    if (collection.count() <= 0)
        return QWebElement();

    foreach(QWebElement inputElement, collection)
    {
        QString id = inputElement.attribute("id", "");
        if (id == "" || !inputElement.hasFocus())
            continue;
        return inputElement;
    }
    return QWebElement();
}

QString MainWindow::getFocusedInputText()
{
    QWebElement focus = getFocusedInputElement();
    if (focus == QWebElement())
        return "";
    return focus.evaluateJavaScript("this.value").toString();
}

QString MainWindow::getFocusedInputID()
{
    QWebElement focus = getFocusedInputElement();
    if (focus == QWebElement())
        return "";
    return focus.attribute("id", "");
}

bool MainWindow::updateFocusedInputScreenshot(bool forced /* = false */)
{
    //Note: This is not very efficient to keep creating & deleting the screenshot file
    //      but it is simplest & stateless (not much dependency on NeTVServer) to implement this
    //      besides it is not hurting too much considered the file is in RAM partition

    //If we have a different focused element, trigger an event to NeTVServer
    QWebElement newFocus = getFocusedInputElement();
    QString newValue = (newFocus == QWebElement()) ? "" : newFocus.evaluateJavaScript("this.value").toString();
    QString newID = (newFocus == QWebElement()) ? "" : newFocus.attribute("id", "");

    if (!forced && this->focusedInput == newFocus && this->focusedInputValue == newValue && this->focusedInputID == newID)
        return false;

    this->focusedInput = newFocus;
    this->focusedInputID = newID;
    this->focusedInputValue = newValue;

    //Delete the screenshot if no focus
    if (newFocus == QWebElement()) {
        UnlinkFile(INPUT_SCREENSHOT_FILE);
        return false;
    }

    //Take screenshot, save to /tmp
    QRect rect = this->focusedInput.geometry();
    int dw = 200 - rect.width()/2;
    int dh = 150 - rect.height()/2;
    rect.setLeft(rect.left()-dw);
    rect.setTop(rect.top()-dh);
    rect.setWidth(400);
    rect.setHeight(300);

    if (rect.x() < 0)                                   rect.moveLeft(0);
    if (rect.y() < 0)                                   rect.moveTop(0);
    if (rect.bottom() > this->geometry().bottom())      rect.moveBottom(this->geometry().bottom());
    if (rect.right() > this->geometry().right())        rect.moveRight(this->geometry().right());

    bool returnValue = (QPixmap::grabWidget(this, rect)).save(INPUT_SCREENSHOT_FILE, "png");
    sendFocusedInput(this->focusedInputID.toLatin1(), this->focusedInputValue.toLatin1());

    return returnValue;
}
