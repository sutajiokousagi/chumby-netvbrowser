#ifndef MYWEBPAGE_H
#define MYWEBPAGE_H

#include <QWebPage>

class MyWebPage : public QWebPage
{
    Q_OBJECT

public:

    MyWebPage(QObject *parent = 0);

protected:

    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);

};

#endif // MYWEBPAGE_H
