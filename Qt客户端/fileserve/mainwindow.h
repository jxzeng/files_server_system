#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "common.h"
#include "consumer.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 显示主窗口
    void showMainWindow();
    // 处理信号
    void managerSignals();
    // 重新登陆
    void loginAgain();

signals:
    // 切换用户按钮信号
    void changeUser();
    void setUser(QString user);

public slots:
    void dealsignal(QString str);
private:
    Ui::MainWindow *ui;
    Common m_common;    
    consumer csumer_1;
    consumer csumer_2;
    consumer csumer_3;
};

#endif // MAINWINDOW_H
