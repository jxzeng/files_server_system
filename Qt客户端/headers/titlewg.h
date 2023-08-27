#ifndef TITLEWG_H
#define TITLEWG_H

#include <QWidget>

namespace Ui {
class TitleWg;
}

class TitleWg : public QWidget
{
    Q_OBJECT

public:
    explicit TitleWg(QWidget *parent = nullptr);
    ~TitleWg();

private:
    Ui::TitleWg *ui;
};

#endif // TITLEWG_H
