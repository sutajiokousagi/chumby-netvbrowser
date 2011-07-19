#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QStringList>
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

#define ARGS_SPLIT_TOKEN    "|~|"
#define DEFAULT_HOST_URL    "localhost"
#define DEFAULT_PORT        8081
#define UNIMPLEMENTED       "Un1mPl3m3nT3D"

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

private slots:

    void slot_socketDisconnected();
    void slot_socketConnected();
    void slot_socketError(QAbstractSocket::SocketError);
    void slot_sockerRetry();
    void slot_socketReadReady();

    void slot_notifyBrowser();
    void slot_newSocketMessage(SocketRequest *request, SocketResponse *response );
};

#endif // MAINWINDOW_H
