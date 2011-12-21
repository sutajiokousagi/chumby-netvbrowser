#include "mainwindow.h"
#include "ui_mainwindow.h"

bool MainWindow::hasSettingsFile()
{
    return FileExists(SETTINGS_FILE);
}

bool MainWindow::reloadSettings()
{
    if (!hasSettingsFile())
        return false;

    QSettings browserSettings(SETTINGS_FILE, QSettings::IniFormat);
    browserSettings.beginGroup("Main");
    enNativeKeyboard = browserSettings.value("enNativeKeyboard", enNativeKeyboard).toBool();
    enMouseCursor = browserSettings.value("enMouseCursor", enMouseCursor).toBool();
    enKeepAliveTimer = browserSettings.value("enKeepAliveTimer", enKeepAliveTimer).toBool();
    keepAliveInterval = browserSettings.value("keepAliveInterval", keepAliveInterval).toInt();
    enJavaScriptConsoleLog = browserSettings.value("enJavaScriptConsoleLog", enJavaScriptConsoleLog).toBool();
    return true;
}

bool MainWindow::saveSettings()
{
    QSettings browserSettings(SETTINGS_FILE, QSettings::IniFormat);
    browserSettings.beginGroup("Main");
    browserSettings.setValue("enJavaScriptConsoleLog", enJavaScriptConsoleLog);
    browserSettings.setValue("enNativeKeyboard", enNativeKeyboard);
    browserSettings.setValue("enMouseCursor", enMouseCursor);
    browserSettings.setValue("enKeepAliveTimer", enKeepAliveTimer);
    browserSettings.setValue("keepAliveInterval", keepAliveInterval);
    browserSettings.endGroup();
    browserSettings.sync();
}
