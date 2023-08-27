#ifndef LOGIN_H
#define LOGIN_H
#include "common.h"
#include <mainwindow.h>
#include <QDialog>
#include<QMouseEvent>
#include <QString>
#include<QMessageBox>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<QFile>
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QNetworkRequest>
#include "Singleton.h"
#include "des.h"
namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *ev);


signals:
    void showSetWg();
    void closeWindow();
private slots:
    void on_Registration_clicked();

    void on_pushButton_2_clicked();
    QByteArray setLoginJson(QString user, QString pwd);
    QStringList getLoginStatus(QByteArray json);
    void on_pushButton_clicked();
    void saveWebinfo(QString ip,QString port, QString path);
    void on_pushButton_ok_clicked();
    void fill_info();
    void on_back_2_clicked();

    void on_back_1_clicked();

private:
    Ui::login *ui;
    QPoint m_pt; //鼠标当前位置-窗口左上角点
    QWidget* m_parent;
    Common m_cm;
    MainWindow m_mainWin;
};

#endif // LOGIN_H
