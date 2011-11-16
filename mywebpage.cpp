#include "mywebpage.h"
#include <QDebug>

MyWebPage::MyWebPage(QObject *parent) : QWebPage(parent)
{
}

void MyWebPage::javaScriptAlert ( QWebFrame * frame, const QString & msg )
{
    Q_UNUSED(frame);

    //Disable pop-up window, just print out to console
    qDebug() << "NeTVBrowser: javaScriptAlert: " << msg;
}

bool MyWebPage::javaScriptConfirm ( QWebFrame * frame, const QString & msg )
{
    Q_UNUSED(frame);

    //Disable pop-up window, just print out to console
    qDebug() << "NeTVBrowser: javaScriptConfirm: " << msg;
    return false;
}

void MyWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    if (message.startsWith("|~|"))      qDebug() << message.right(message.length()-3);
    else                                qDebug() << sourceID << " (line " << lineNumber << "): " << message;
}

bool MyWebPage::javaScriptPrompt ( QWebFrame * /* frame */, const QString & msg, const QString & defaultValue, QString * result )
{
    Q_UNUSED(defaultValue);
    Q_UNUSED(result);

    //Disable pop-up window, just print out to console
    qDebug() << "NeTVBrowser: javaScriptPrompt: " << msg;
    return false;
}

bool MyWebPage::swallowContextMenuEvent ( QContextMenuEvent * event )
{
    Q_UNUSED(event);
    qDebug() << "NeTVBrowser: swallow context menu";
    return true;
}

bool MyWebPage::shouldInterruptJavaScript()
{
    qDebug() << "NeTVBrowser: unresponsive JavaScript detected. Killed.";
    return true;
}

QString MyWebPage::chooseFile ( QWebFrame * parentFrame, const QString & suggestedFile )
{
    Q_UNUSED(parentFrame);
    Q_UNUSED(suggestedFile);
    return "";
}
