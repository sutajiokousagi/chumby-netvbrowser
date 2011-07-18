#include "mywebpage.h"
#include <QDebug>

MyWebPage::MyWebPage(QObject *parent) : QWebPage(parent)
{
}

void MyWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    if (message.startsWith("|~|"))      qDebug() << message.right(message.length()-3);
    else                                qDebug() << sourceID << " (line " << lineNumber << "): " << message;
}
