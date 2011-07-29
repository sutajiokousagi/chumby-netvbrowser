#ifndef MYWEBVIEW_H
#define MYWEBVIEW_H

#include <QWebView>

class MyWebView : public QWebView
{
    Q_OBJECT
public:

    explicit MyWebView(QWidget *parent = 0);

    void setInvertColor(bool isInverted = false)    {   this->invertColor = isInverted;    }
    bool getInvertColor()                           {   return this->invertColor;          }

protected:

    void paintEvent( QPaintEvent *pe );

private:

    bool invertColor;

};

#endif // MYWEBVIEW_H
