#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mywebview.h"

namespace Ui {
    class MainWindow;
}

#define ARGS_SPLIT_TOKEN    "|~|"
#define DEFAULT_URL         "http://www.chumby.com"

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
    MyWebView* myWebView;
};

#endif // MAINWINDOW_H
