#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QStringList>
#include <QKeyEvent>
#include "mywebview.h"
#include "mywebpage.h"
#include "socketrequest.h"
#include "socketresponse.h"

namespace Ui {
    class MainWindow;
}

#ifndef Q_WS_QWS
#define SUPERVERBOSE
#endif

/** Name of this application */
#define APPNAME "NeTVBrowser"

/** Publisher of this application */
#define ORGANISATION "Chumby Industries"

/** Short description of this application */
#define DESCRIPTION "Customized web server based on Qt"

/** The special string used to split & join arguements */
#define ARGS_SPLIT_TOKEN    "|~|"

#define DEFAULT_HOST_URL    "localhost"
#define DEFAULT_PORT        8081
#define UNIMPLEMENTED       "Un1mPl3m3nT3D"
#define TAG                 APPNAME

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:

    void receiveArgs(const QString &argsString);

private:

    Ui::MainWindow *ui;

    //Flags
    bool isShuttingDown;

    //Webview
    bool cPanelLoaded;
    MyWebView* myWebView;
    MyWebPage* myWebPage;
    void setupWebview();
    void resetWebview();

    //Communication
    quint16 port;
    QTcpSocket *mySocket;
    void setupSocket();

    //Common functions
    void sendSocketHello(SocketResponse *response);
    Qt::Key getKeyCode(QString keyname);
    QByteArray processStatelessCommand(QByteArray command, QStringList argsList = QStringList());

    //Remote control & Keyboard events
    QByteArray remoteControlKey(QByteArray buttonName);
    qint64 up,down,left,right,center,cpanel,widget,hidden1,hidden2;

protected:

    void keyPressEvent ( QKeyEvent * event );
    void keyReleaseEvent  ( QKeyEvent * event );

private slots:

    void slot_pageloadStarted();
    void slot_pageloadFinished(bool ok);
    void slot_pageloadProgress(int progress);

    void slot_socketDisconnected();
    void slot_socketConnected();
    void slot_socketError(QAbstractSocket::SocketError);
    void slot_sockerRetry();
    void slot_socketReadReady();

    void slot_notifyBrowser();
    void slot_newSocketMessage(SocketRequest *request, SocketResponse *response );
};

#endif // MAINWINDOW_H
