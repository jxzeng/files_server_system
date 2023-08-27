#include "titlewg.h"
#include "ui_titlewg.h"

TitleWg::TitleWg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleWg)
{
    ui->setupUi(this);
    //logo图片
    ui->logo->setPixmap(QPixmap(":/images/logo.ico"));
}

TitleWg::~TitleWg()
{
    delete ui;
}
