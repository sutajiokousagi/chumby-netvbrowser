#ifndef MYWEBPAGE_H
#define MYWEBPAGE_H

#include <QWebPage>

class MyWebPage : public QWebPage
{
    Q_OBJECT

public:

    MyWebPage(QObject *parent = 0);

    bool enJavascriptConsoleLog;

protected:

    void javaScriptAlert ( QWebFrame * frame, const QString & msg );
    bool javaScriptConfirm ( QWebFrame * frame, const QString & msg );
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
    bool javaScriptPrompt ( QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result );
    bool swallowContextMenuEvent ( QContextMenuEvent * event );
    QString chooseFile ( QWebFrame * parentFrame, const QString & suggestedFile );

public slots:

    bool shouldInterruptJavaScript();

};

#endif // MYWEBPAGE_H
