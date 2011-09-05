#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QStringList>
#include <QKeyEvent>
#include <QFile>
#include <QByteArray>
#include <QMap>
#include <QTimer>
#include "mywebview.h"
#include "mywebpage.h"
#include <QWebFrame>
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
#define ARGS_SPLIT_TOKEN        "|~|"

#define DEFAULT_HOST_URL        "localhost"
#define DEFAULT_PORT            8081
#define UNIMPLEMENTED           "Un1mPl3m3nT3D"
#define TAG                     APPNAME

#define OPKG_READ_INTERVAL      2000
#define UPGRADE_SCRIPT          "/usr/bin/chumby-netvbrowser-upgrade.sh"
#define OPKG_DOWNLOAD_PATH      "/var/lib/opkg/tmp"
#define UPGRADE_PROGRESS_FILE   "/tmp/netvbrowser_temp_upgrade"
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
    QWebFrame* myIFrame;
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
    QMap<QByteArray, QByteArray> packageSizeMap;
    qint64 doUpgrade(bool reboot);
    void upgradeDone();
    void setupUpgrade();
    void resetUpgrade();
    QByteArray getFriendlyPackageName(QByteArray rawName);
    void getUpgradablePackageList();
    void getDownloadedPackageSize();
    QByteArray updatePackageSize(QByteArray rawPackageName, quint64 size);
    QByteArray updatePackageState(QByteArray packagefilename, bool isDone);
    quint64 getPackageSize(QByteArray packagefilename);
    bool getPackageState(QByteArray packagefilename);
    quint64 getUpdateTotalSizeKb();
    quint64 getUpdateProgressSizeKb();
    double getUpdatePercentage();

    bool exportPackageList();
    bool importPackageList();

protected:

    void keyPressEvent ( QKeyEvent * event );
    void keyReleaseEvent  ( QKeyEvent * event );
    void paintEvent( QPaintEvent *pe );

private slots:

    void slot_pageloadStarted();
    void slot_pageloadFinished(bool ok);
    void slot_pageloadProgress(int progress);
    void slot_statusBarMessage ( const QString & text );

    void slot_frameCreated(QWebFrame*);
    void slot_frameLoadFinished(bool);
    void slot_frameContentSizeChange(const QSize&);

    void slot_socketDisconnected();
    void slot_socketConnected();
    void slot_socketError(QAbstractSocket::SocketError);
    void slot_socketRetry();
    void slot_socketReadReady();

    void slot_notifyBrowser();
    void slot_newSocketMessage(SocketRequest *request, SocketResponse *response );
};

#endif // MAINWINDOW_H
