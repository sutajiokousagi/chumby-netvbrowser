#include "mywebview.h"

MyWebView::MyWebView(QWidget *parent) :
    QWebView(parent)
{
    invertColor = false;
}

void MyWebView::paintEvent(QPaintEvent *pe)
{
    // normal painting
    QWebView::paintEvent(pe);

    // invert the color
    if (invertColor)
    {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Difference);
        p.fillRect(this->rect(), QColor(255,255,255,255));
    }    
}
