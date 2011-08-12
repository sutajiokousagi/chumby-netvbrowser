#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QStringList>
#include <QKeyEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QByteArray>
#include <QMap>
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

#define DEFAULT_HOST_URL        "localhost"
#define DEFAULT_PORT            8081
#define UNIMPLEMENTED           "Un1mPl3m3nT3D"
#define TAG                     APPNAME
#define OPKG_FIFO               "/tmp/opkg_upgrade_fifo"
#define UPDATE_PROGRESS_FILE    "/tmp/netvbrowser_temp_upgrade"
#define OPKG_DOWNLOAD_PATH      "/var/lib/opkg/tmp"
#define UPDATE_PAGE             "http://localhost/html_update/index.html"

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
    void resetWebview(QByteArray address = "");

    //File Utilities
    bool FileExists(const QString &fullPath);
    bool FileExecutable(const QString &fullPath);
    qint64  GetFileSize(const QString &fullPath);
    QByteArray GetFileContents(const QString &fullPath);
    bool SetFileContents(const QString &fullPath, QByteArray data);
    bool SetFileExecutable(const QString &fullPath);
    bool UnlinkFile(const QString &fullPath);

    // Process Utilities
    QByteArray Execute(const QString &fullPath);
    QByteArray Execute(const QString &fullPath, QStringList args);

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

    //Update mechanism
    QLocalServer *opkgSocket;
    QList<QByteArray> packageList;
    QMap<QByteArray, quint64> packageSizeMap;
    QMap<QByteArray, bool> packageStateMap;
    qint64 doUpgrade();
    void setupUpdate();
    void resetUpdate();
    QByteArray getFriendlyPackageName(QByteArray rawName);
    void getUpgradablePackageList();
    void getDownloadedPackageSize();
    void updatePackageSize(QByteArray rawPackageName, quint64 size);
    void updatePackageState(QByteArray packagefilename, bool isDone);
    quint64 getUpdateTotalSizeKb();
    quint64 getUpdateProgressSizeKb();
    double getUpdatePercentage();

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
    void slot_socketRetry();
    void slot_socketReadReady();

    void slot_notifyBrowser();
    void slot_newSocketMessage(SocketRequest *request, SocketResponse *response );

    void slot_opkgReceiveConnection();
    void slot_opkgConnected();
    void slot_opkgDisconnected();
    void slot_opkgError(QLocalSocket::LocalSocketError err);
    void slot_opkgReadReady();
};

#endif // MAINWINDOW_H
