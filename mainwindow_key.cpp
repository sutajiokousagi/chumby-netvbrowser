#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>

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
        case Qt::Key_HomePage:
            //Will be delivered 1 second later
            addKeyStrokeHistory("setup");
            return;

        case Qt::Key_Up:
            up = currentEpochMs;
            remoteControlKey("up");
            addKeyStrokeHistory("up");
            remoteControlPageInteraction("up");
            return;
        case Qt::Key_Down:
            down = currentEpochMs;
            remoteControlKey("down");
            addKeyStrokeHistory("down");
            remoteControlPageInteraction("down");
            return;
        case Qt::Key_Left:
            left = currentEpochMs;
            remoteControlKey("left");
            addKeyStrokeHistory("left");
            return;
        case Qt::Key_Right:
            right = currentEpochMs;
            remoteControlKey("right");
            addKeyStrokeHistory("right");
            return;

        case Qt::Key_Enter:
        case Qt::Key_Return:
            center = currentEpochMs;
            remoteControlKey("center");
            addKeyStrokeHistory("center");
            return;
        case Qt::Key_PageUp:
            cpanel = currentEpochMs;
            remoteControlKey("cpanel");
            addKeyStrokeHistory("cpanel");
            return;
        case Qt::Key_PageDown:
            widget = currentEpochMs;
            remoteControlKey("widget");
            addKeyStrokeHistory("widget");
            return;

        case Qt::Key_1:
            hidden1 = currentEpochMs;
            remoteControlKey("reset");
            addKeyStrokeHistory("hidden1");
            return;
        case Qt::Key_2:
            hidden2 = currentEpochMs;
            remoteControlKey("reset");
            addKeyStrokeHistory("hidden2");
            return;
    }

    qDebug("%s: keyPressEvent '%d'", TAG, keycode);

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

void MainWindow::addKeyStrokeHistory(QString keyName)
{
    //Disable this temporarily, seems to cause the browser to hang randomly
    if (keyName.toUpper() != "SETUP")
        return;

    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();
    keyStrokeHistory.prepend( QString("%1|%2").arg(QString(keyName)).arg(currentEpochMs) );
    while (keyStrokeHistory.size() > 16)
        keyStrokeHistory.removeLast();

    //Delayed action for SETUP button
    if (keyName.toUpper() == "SETUP") {
        if (!keyStrokeTimer.isActive())
            keyStrokeTimer.start();
        return;
    }

    //Detect complex pattern
    /*
    if (keyStrokeHistory.size() >= 9)
    {
        QString keyString;
        bool hit = false;
        for (int i=0; i < 9; i++)
            keyString = QString(keyStrokeHistory.at(i).split("|").at(0)).append(QString(",")).append(keyString);

        if (keyString.contains("up,right,down,left,up,right,down,left,cpanel")) {
            qDebug("%s: key combo: %s", TAG, keyString.toLatin1().constData());
        }
        else if (keyString.contains("up,right,down,left,up,right,down,left,widget")) {
            qDebug("%s: key combo: %s", TAG, keyString.toLatin1().constData());
        }
        if (hit)
            keyStrokeHistory.clear();
    }
    */
}

void MainWindow::slot_keyStrokeTimeout()
{
    keyStrokeTimer.stop();
    qint64 currentEpochMs = QDateTime::currentMSecsSinceEpoch();

    //Get the keys pressed within 2 seconds
    QStringList tempOneSecList;
    for (int i=0; i < keyStrokeHistory.size(); i++)
    {
        QString temp = keyStrokeHistory.at(i);
        bool ok = false;
        qint64 time = temp.split("|").at(1).toLongLong(&ok);
        if (!ok)
            continue;
        if (currentEpochMs - time > 2000)
            continue;
        tempOneSecList.prepend(temp);
    }

    //Counting which keys are pressed within 2 second
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
        if (key == "setup")
        {
            if (count > 0)
            {
                //Call a system script to force rekey & hotplug on FPGA
                this->Execute("/usr/bin/fpga_setup", QStringList() << QString().setNum(count));

                //Deliver event to Control Panel
                remoteControlKey(key.toLatin1(), count);
            }
        }
        else if (count > 1)
            remoteControlKey(key.toLatin1(), count);
    }

    tempOneSecList.clear();
    keyCountMap.clear();
}

void MainWindow::remoteControlPageInteraction(QString buttonName)
{
    if (isWebViewTabVisible(SECOND_TAB))
    {
        QSize frameSize = this->frameGeometry().size();
        if (buttonName == "up")             scrollWebViewTabDelta(SECOND_TAB, 0, -frameSize.height() / 7);
        else if (buttonName == "down")      scrollWebViewTabDelta(SECOND_TAB, 0, frameSize.height() / 7);
    }
}
