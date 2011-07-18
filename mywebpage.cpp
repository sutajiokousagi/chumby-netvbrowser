#include "mywebpage.h"
#include <QDebug>

MyWebPage::MyWebPage(QObject *parent) : QWebPage(parent)
{
}

void MyWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    QString logText = sourceID + "(line " + QString::number(lineNumber) + "): " + message;
    qDebug() << logText;
}
