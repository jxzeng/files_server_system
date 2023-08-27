#include "main_topwidget.h"
#include "ui_main_topwidget.h"

main_topwidget::main_topwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::main_topwidget)
{
    ui->setupUi(this);
}

main_topwidget::~main_topwidget()
{
    delete ui;
}
// 当前登录用户初始化
void main_topwidget::changeCurrUser(QString user)
{
    ui->login_user->setText(user);
}
