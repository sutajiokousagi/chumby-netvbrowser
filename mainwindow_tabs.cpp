#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>

void MainWindow::initWebViewTab(int index)
{
    qDebug("%s: initWebViewTab %d", TAG, index);

    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] != NULL)
        deinitWebViewTab(index);

    myWebViewArray[index] = new MyWebView(this);
    this->ui->rootLayout->addWidget(myWebViewArray[index]);

    //Disable Anti-aliasing
    myWebViewArray[index]->setRenderHints(0);

    //Do any other customization on default page
    MyWebPage *newPage = new MyWebPage(this);
    myWebViewArray[index]->setInvertColor(false);
    myWebViewArray[index]->setPage(newPage);

    //Hide scrollbars
    myWebViewArray[index]->page()->mainFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    myWebViewArray[index]->page()->mainFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Set reference pointer to default tab for convenience
    if (index == DEFAULT_TAB)
        this->myWebView = myWebViewArray[DEFAULT_TAB];
}

void MainWindow::deinitWebViewTab(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    qDebug("%s: deinitWebViewTab %d", TAG, index);

    //Delete QWebView object
    myWebViewArray[index]->setHtml("");
    myWebViewArray[index]->setVisible(false);
    delete myWebViewArray[index];
    myWebViewArray[index] = NULL;
}

void MainWindow::resetAllTab()
{
    for (int i=0;i<MAX_TABS; i++)
        if (i != DEFAULT_TAB)
            hideWebViewTab(i);

    this->resetWebViewTab(DEFAULT_TAB);
    this->showWebViewTab(DEFAULT_TAB);
}

void MainWindow::resetWebViewTab(int index, QByteArray address /* = "" */)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    qDebug("NeTVBrowser: resetWebViewTab %d", index);

    //Transparent background (page content dependent)
    QPalette palette = myWebViewArray[index]->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    myWebViewArray[index]->page()->setPalette(palette);
    myWebViewArray[index]->setAttribute(Qt::WA_OpaquePaintEvent, false);

    //Not special tab
    if (index != 0)
    {
        myWebViewArray[index]->load( QUrl(address, QUrl::TolerantMode) );
        return;
    }

    //Connect signal
    QObject::connect(myWebViewArray[index]->page(), SIGNAL(loadFinished(bool)), this, SLOT(slot_pageloadFinished(bool)));
    QObject::connect(myWebViewArray[index]->page(), SIGNAL(loadStarted()), this, SLOT(slot_pageloadStarted()));
    QObject::connect(myWebViewArray[index]->page(), SIGNAL(loadProgress(int)), this, SLOT(slot_pageloadProgress(int)));
    QObject::connect(myWebViewArray[index]->page(), SIGNAL(frameCreated(QWebFrame*)), this, SLOT(slot_frameCreated(QWebFrame*)));
    QObject::connect(myWebViewArray[index], SIGNAL(statusBarMessage(QString)), this, SLOT(slot_statusBarMessage(QString)));

    //Check if we have a custom homepage URL
    QString homepageUrl = QString("http://%1").arg(DEFAULT_HOST_URL);
    if (address != "")
    {
        homepageUrl = QString(address);
    }
    else if (FileExists(HOMEPAGE_PAGE_FILE))
    {
        QString temp_homepageUrl = QString(GetFileContents(HOMEPAGE_PAGE_FILE).trimmed());
        if (temp_homepageUrl.length() > 10 && (temp_homepageUrl.contains("http://") || temp_homepageUrl.contains("https://") || temp_homepageUrl.contains("index.html")))
            homepageUrl = temp_homepageUrl;
    }

    //Load default page
    if (index == DEFAULT_TAB)
        qDebug("Homepage URL: %s", qPrintable(homepageUrl));
    myWebViewArray[index]->load( QUrl(homepageUrl, QUrl::TolerantMode) );

    //Remember to do this at calling function
    //showWebViewTab(index);
}

void MainWindow::loadWebViewTab(int index, QByteArray address /* = "" */)
{
    if (index < 0 || index >= MAX_TABS || address.length() < 8)
        return;
    if (myWebViewArray[index] == NULL)
        initWebViewTab(index);

    this->resetWebViewTab(index, address);
    this->showWebViewTab(index);
}

void MainWindow::loadWebViewTabHTML(int index, QByteArray htmlString /* = "" */)
{
    if (index < 0 || index >= MAX_TABS || htmlString.length() < 2)
        return;
    if (myWebViewArray[index] == NULL)
        initWebViewTab(index);

    this->resetWebViewTab(index, " ");
    this->myWebViewArray[index]->setHtml(htmlString);
    this->showWebViewTab(index);
}

void MainWindow::showWebViewTab(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    qDebug("NeTVBrowser: showWebViewTab %d", index);
    this->currentWebViewTab = index;

    //Note: under non-native keyboard mode, all tabs are setEnabled(false) so that they don't catch any event
    //only MainWindow is enabled hence catchesr all events regardless of which tab is being shown

    //Show it
    myWebViewArray[index]->setVisible(true);
    myWebViewArray[index]->setEnabled(enNativeKeyboard);
    myWebViewArray[index]->setFocus(Qt::MouseFocusReason);

    //Resize fullscreen
    myWebViewArray[index]->resize(this->frameGeometry().size());
    myWebViewArray[index]->move(0,0);
}

void MainWindow::hideWebViewTab(int index, bool destroy /*= false*/)
{
    if (myWebViewArray[index] == NULL)
        return;
    qDebug("NeTVBrowser: hideWebViewTab %d", index);

    myWebViewArray[index]->setVisible(false);
    myWebViewArray[index]->setEnabled(false);
    myWebViewArray[index]->clearFocus();

    //Clear memory if not main tab
    if (index != DEFAULT_TAB && destroy)
        deinitWebViewTab(index);
}

void MainWindow::hideOtherWebViewTab(int index, bool destroy /*= false*/)
{
    if (index < 0 || index >= MAX_TABS)
        return;

    for (int i=0; i<MAX_TABS; i++)
        if (i != index)
            hideWebViewTab(i, destroy);
}

QSize MainWindow::getWebViewTabContentSize(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return QSize();
    if (myWebViewArray[index] == NULL)
        return QSize();
    return myWebViewArray[index]->page()->mainFrame()->contentsSize();
}

bool MainWindow::isWebViewTabVisible(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return false;
    if (myWebViewArray[index] == NULL)
        return false;
    return myWebViewArray[index]->isVisible();
}

void MainWindow::scrollWebViewTabDelta(int index, int dx, int dy)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    myWebViewArray[index]->page()->mainFrame()->scroll(dx,dy);
}

void MainWindow::scrollWebViewTabAbsolute(int index, int x, int y)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    myWebViewArray[index]->page()->mainFrame()->setScrollPosition(QPoint(x,y));
}

void MainWindow::scrollWebViewTabPercentage(int index, double x, double y)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;

    QSize contentSize = getWebViewTabContentSize(index);
    int absx = contentSize.width() * x;
    int absy = contentSize.height() * y;
    myWebViewArray[index]->page()->mainFrame()->setScrollPosition(QPoint(absx,absy));
}

void MainWindow::sendWebViewTabEvent(int index, QEvent * event)
{
    //QApplication::sendEvent(this->centralWidget(), event);
    //return;

    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    QApplication::sendEvent(myWebViewArray[index], event);
}
