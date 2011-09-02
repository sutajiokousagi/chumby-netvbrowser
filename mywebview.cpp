#include "mywebview.h"

MyWebView::MyWebView(QWidget *parent) :
    QWebView(parent)
{
    invertColor = false;
}

void MyWebView::paintEvent(QPaintEvent *pe)
{
    //This doesn't seems to be doing any good
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::NonCosmeticDefaultPen, false);

    // normal painting
    QWebView::paintEvent(pe);

    // invert the color
    if (invertColor)
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Difference);
        painter.fillRect(this->rect(), QColor(255,255,255,255));
    }       
}
