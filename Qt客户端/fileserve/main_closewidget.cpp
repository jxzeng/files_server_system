#include "main_closewidget.h"
#include "ui_main_closewidget.h"
#include <QMouseEvent>
#include <QPainter>
main_closewidget::main_closewidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::main_closewidget)
{
    ui->setupUi(this);
    connect(ui->max,&QToolButton::clicked,[=](){
        static bool fl = false;
        if(!fl)
        {
            ui->max->setIcon(QIcon(":/images/title_normal.png"));
            m_parent->showMaximized();
            fl = true;
        }
        else
        {
            ui->max->setIcon(QIcon(":/images/title_max.png"));
            m_parent->showNormal();
            fl = false;
        }

    });
    connect(ui->min,&QToolButton::clicked,[=](){
        m_parent->showMinimized();
    });
    connect(ui->close,&QToolButton::clicked,[=](){
        m_parent->close();
    });
}

main_closewidget::~main_closewidget()
{
    delete ui;
}

void main_closewidget::setParent(QWidget *parent)
{
    m_parent = parent;
}

void main_closewidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPixmap pixmap(":/images/closewidget.png");
    painter.drawPixmap(0, 0, width(), height(), pixmap);
}

void main_closewidget::mousePressEvent(QMouseEvent *event)
{
    // 如果是左键, 计算窗口左上角, 和当前按钮位置的距离
    if(event->button() == Qt::LeftButton)
    {
        // 计算和窗口左上角的相对位置
        m_pos = event->globalPos() - m_parent->geometry().topLeft();
    }
}

void main_closewidget::mouseMoveEvent(QMouseEvent *event)
{
    // 移动是持续的状态, 需要使用buttons
    if(event->buttons() & Qt::LeftButton)
    {
        QPoint pos = event->globalPos() - m_pos;
        m_parent->move(pos);
    }
}
