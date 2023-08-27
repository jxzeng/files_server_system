#ifndef MAINWINDOWS_RIGHTWIDGET_H
#define MAINWINDOWS_RIGHTWIDGET_H

#include <QWidget>
#include <QSignalMapper>
#include <QMap>
namespace Ui {
class mainwindows_rightwidget;
}
class QToolButton;
enum Page{MYFILE, SHARE, TRANKING, TRANSFER, SWITCHUSR};
class mainwindows_rightwidget : public QWidget
{
    Q_OBJECT

public:
    explicit mainwindows_rightwidget(QWidget *parent = nullptr);
    ~mainwindows_rightwidget();
    void changeCurrUser(QString user);

public slots:
    // 按钮处理函数
    void slotButtonClick(Page cur);
    void slotButtonClick(QString text);
    void setParent(QWidget *parent);


signals:
    void sigMyFile();
    void sigShareList();
    void sigDownload();
    void sigTransform();
    void sigSwitchUser();


private:
    Ui::mainwindows_rightwidget *ui;
    QPoint m_pos;
    QWidget* m_parent;
    QSignalMapper* m_mapper;
    QToolButton* m_curBtn;
    QMap<QString, QToolButton*> m_btns;
    QMap<Page, QString> m_pages;
};

#endif // MAINWINDOWS_RIGHTWIDGET_H
