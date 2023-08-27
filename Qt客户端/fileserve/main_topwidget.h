#ifndef MAIN_TOPWIDGET_H
#define MAIN_TOPWIDGET_H

#include <QWidget>

namespace Ui {
class main_topwidget;
}

class main_topwidget : public QWidget
{
    Q_OBJECT

public:
    explicit main_topwidget(QWidget *parent = nullptr);
    ~main_topwidget();
    void changeCurrUser(QString user);

private:
    Ui::main_topwidget *ui;
};

#endif // MAIN_TOPWIDGET_H
