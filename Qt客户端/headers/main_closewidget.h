#ifndef MAIN_CLOSEWIDGET_H
#define MAIN_CLOSEWIDGET_H

#include <QWidget>

namespace Ui {
class main_closewidget;
}

class main_closewidget : public QWidget
{
    Q_OBJECT

public:
    explicit main_closewidget(QWidget *parent = nullptr);
    ~main_closewidget();
    void setParent(QWidget *parent);
protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent* event);

private:
    Ui::main_closewidget *ui;
    QPoint m_pos;
    QWidget* m_parent;
};

#endif // MAIN_CLOSEWIDGET_H
