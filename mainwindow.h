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
#include <QWebFrame>
#include <QWebElement>
#include <QSettings>

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
#define ARGS_SPLIT_TOKEN        "|~|"

//Hardcoded, not configurable
#define DEFAULT_HOST_URL        "localhost"
#define DEFAULT_PORT            8081
#define UNIMPLEMENTED           "Un1mPl3m3nT3D"
#define TAG                     APPNAME
#define MAX_TABS                10
#define DEFAULT_TAB             0
#define SECOND_TAB              1
#define FOCUS_INPUT_TIMEOUT     2000
#define KEY_TIMEOUT             1800

#define KEEPALIVE_INTERVAL      40000
#define ENABLE_FSERVERRESET     false
#define ENABLE_NATIVE_KB        false
#define ENABLE_MOUSE_CURSOR     false
#define ENABLE_KEEPALIVE        true
#define ENABLE_JAVASCRIPT_LOG   true

#define UPGRADE_SCRIPT          "/usr/bin/chumby-netvbrowser-upgrade.sh"
#define OPKG_DOWNLOAD_PATH      "/var/lib/opkg/tmp"
#define UPGRADE_PROGRESS_FILE   "/tmp/netvbrowser_upgrade_packages"
#define UPDATE_PAGE             "http://localhost/html_update/index.html"
#define FACTORY_PAGE            "http://localhost/tests/index.html"
#define SETTINGS_FILE           "/psp/NeTVBrowser.ini"
#define HOMEPAGE_PAGE_FILE      "/psp/homepage"
#define CPANEL_GIT_LOG          "/tmp/cpanel_git.log"
#define INPUT_SCREENSHOT_FILE   "/tmp/focused_input.png"

#define HTML_IMAGE              "<html><body style='margin:0; overflow:hidden;'><table width='100%' height='100%' cell-padding='0' cell-spacing='0'><tr><td width='100%' height='100%' align='center' valign='middle'><img src='xxxxxxxxxx' /></tr></td></table></body></html>"

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

    //Custom docroot
    QString checkPspHomepageLocalPath();

    //Flags
    bool isShuttingDown;
    bool enNativeKeyboard;
    bool enMouseCursor;
    bool enKeepAliveTimer;
    bool enJavaScriptConsoleLog;
    int keepAliveInterval;
    bool updateCPanel;

    bool hasSettingsFile();
    bool reloadSettings();
    bool saveSettings();

    //Webview
    bool cPanelLoaded;
    int cPanelLoadCount;
    MyWebView* myWebView;
    MyWebView* myWebViewArray[MAX_TABS];
    QTimer keepAliveTimer;

    //Multi-tabs
    int currentWebViewTab;
    void initWebViewTab(int index);
    void deinitWebViewTab(int index);
    void resetAllTab();
    void resetWebViewTab(int index, QByteArray address = "");
    void loadWebViewTab(int index, QByteArray address = "");
    void loadWebViewTabHTML(int index, QByteArray htmlString = "");
    void showWebViewTab(int index);
    void hideWebViewTab(int index, bool destroy = false);
    void hideOtherWebViewTab(int index, bool destroy = false);
    QSize getWebViewTabContentSize(int index);
    bool isWebViewTabVisible(int index);
    void scrollWebViewTabDelta(int index, int dx, int dy);
    void scrollWebViewTabAbsolute(int index, int x, int y);
    void scrollWebViewTabPercentage(int index, double x, double y);
    void sendWebViewTabEvent(int index, QEvent * event);

    //File Utilities
    bool FileExists(const QString &fullPath);
    bool FileExecutable(const QString &fullPath);
    qint64  GetFileSize(const QString &fullPath);
    QByteArray GetFileContents(const QString &fullPath);
    bool SetFileContents(const QString &fullPath, QByteArray data);
    bool SetFileExecutable(const QString &fullPath);
    bool UnlinkFile(const QString &fullPath);

    //Javascript Utilities
    bool HasJavaScriptFunction(QString functionName);

    //Other Utilities
    bool IsHexString(QString testString);

    //Process Utilities
    QByteArray Execute(const QString &fullPath);
    QByteArray Execute(const QString &fullPath, QStringList args);

    //Communication
    quint16 port;
    QTcpSocket *mySocket;
    void setupSocket();

    //Communication to NeTVServer
    void sendSocketHello(SocketResponse *response);
    void requestUpdateCPanel();
    void requestSetDocroot(QByteArray newPath);
    void sendFocusedInput(QByteArray id, QByteArray value);
    void sendNeTVServerCommand(QByteArray command);
    void sendNeTVServerCommand(QByteArray command, QMap<QByteArray, QByteArray> params);

    void setURL(QString address);
    Qt::Key getKeyCodeFromName(QString keyname);
    QByteArray getIRKeyName(int keycode);
    QByteArray processStatelessCommand(QByteArray command, QStringList argsList = QStringList());

    //Remote control & Keyboard events
    QMap<int, qint64> keyPressEpochMap;
    QStringList keyStrokeHistory;
    QByteArray remoteControlKey(bool isRepeat, QByteArray buttonName, int oneSecCount = 1);
    QTimer keyStrokeTimer;
    qint64 keyStrokeTimerEpoch;
    void triggerKeycode(int keycode, int count = 1);
    bool addKeyStrokeHistory(QString);
    void remoteControlPageInteraction(int keycode);

    //Reversed textinput event
    QWebElement focusedInput;
    QString focusedInputValue;
    QString focusedInputID;
    QTimer focusInputTimer;
    QWebElement getFocusedInputElement();
    QString getFocusedInputText();
    QString getFocusedInputID();
    bool updateFocusedInputScreenshot(bool forced = false);

    //Update mechanism
    QMap<QByteArray, QByteArray> packageSizeMap;
    qint64 doUpgrade(bool reboot);
    void upgradeDone();
    void setupUpgrade();
    void resetUpgrade();
    QByteArray getFriendlyPackageName(QByteArray rawName);
    QByteArray getFriendlyPackageList();
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

    void resizeEvent ( QResizeEvent * event );
    void keyPressEvent ( QKeyEvent * event );
    void keyReleaseEvent  ( QKeyEvent * event );
    void paintEvent( QPaintEvent *pe );

private slots:

    void slot_pageloadStarted();
    void slot_pageloadFinished(bool ok);
    void slot_pageloadProgress(int progress);
    void slot_statusBarMessage ( const QString & text );

    void slot_frameCreated(QWebFrame*);

    void slot_socketDisconnected();
    void slot_socketConnected();
    void slot_socketError(QAbstractSocket::SocketError);
    void slot_socketRetry();
    void slot_socketReadReady();

    void slot_notifyBrowser();
    void slot_newSocketMessage(SocketRequest *request, SocketResponse *response );

    void slot_keyStrokeTimeout();
    void slot_keepAliveTimeout();
    void slot_requestUpdateCPanel();

    void slot_updateFocusInput();
};

#endif // MAINWINDOW_H
