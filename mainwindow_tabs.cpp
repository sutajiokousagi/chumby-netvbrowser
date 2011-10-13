#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>

void MainWindow::initWebViewTab(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] != NULL)
        return;
    qDebug("%s: initWebViewTab %d", TAG, index);

    myWebViewArray[index] = new MyWebView(this);
    this->ui->rootLayout->addWidget(myWebViewArray[index]);

    //Ignore mouse & keyboard
    if (!ENABLE_NATIVE_KB)      myWebViewArray[index]->setEnabled(false);
    else                        myWebViewArray[index]->setEnabled(index == DEFAULT_TAB);

    //Disable AA
    myWebViewArray[index]->setRenderHints(0);

    //Do any other customization on default page
    myWebPageArray[index] = new MyWebPage(this);
    myWebViewArray[index]->setInvertColor(false);
    myWebViewArray[index]->setPage(myWebPageArray[index]);

    //Hide scrollbars
    myWebViewArray[index]->page()->mainFrame()->setScrollBarPolicy ( Qt::Vertical, Qt::ScrollBarAlwaysOff );
    myWebViewArray[index]->page()->mainFrame()->setScrollBarPolicy ( Qt::Horizontal, Qt::ScrollBarAlwaysOff );

    //Set this as central widget
    //if (index == 0)
    //  this->setCentralWidget(myWebViewArray[index]);
}

void MainWindow::deinitWebViewTab(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return;
    if (myWebViewArray[index] == NULL)
        return;
    qDebug("%s: deinitWebViewTab %d", TAG, index);

    //Remove reference only
    myWebPageArray[index] = NULL;

    //Delete QWebView object
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

    //Load default page
    if (address == "")      myWebViewArray[index]->load( QUrl(QString("http://%1").arg(DEFAULT_HOST_URL)) );
    else                    myWebViewArray[index]->load( QUrl(address, QUrl::TolerantMode) );

    //Show it (remember to do this manually)
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

    //Hide others
    //hideOtherWebViewTab(index);

    //Show it
    myWebViewArray[index]->setVisible(true);
    myWebViewArray[index]->setFocus(Qt::MouseFocusReason);
    currentWebViewTab = index;

    //Resize fullscreen
    myWebViewArray[index]->resize(this->frameGeometry().size());
    myWebViewArray[index]->move(0,0);
}

void MainWindow::hideWebViewTab(int index)
{
    if (myWebViewArray[index] == NULL)
        return;
    myWebViewArray[index]->clearFocus();
    myWebViewArray[index]->setVisible(false);
    qDebug("NeTVBrowser: hideWebViewTab %d", index);

    if (index == DEFAULT_TAB)
        return;

    //Clear memory
    deinitWebViewTab(index);
}

void MainWindow::hideOtherWebViewTab(int index)
{
    if (index < 0 || index >= MAX_TABS)
        return;

    for (int i=0; i<MAX_TABS; i++)
        if (i != index)
            hideWebViewTab(i);
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
