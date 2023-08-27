#ifndef TRANSFER_H
#define TRANSFER_H

#include <QWidget>
#include "common.h"
namespace Ui {
class transfer;
}

class transfer : public QWidget
{
    Q_OBJECT

public:
    explicit transfer(QWidget *parent = nullptr);
    ~transfer();

    // 显示数据传输记录
    void dispayDataRecord(QString path=RECORDDIR);
    // 显示上传窗口
    void showUpload();
    // 显示下载窗口
    void showDownload();

signals:
    void currentTabSignal(QString); // 告诉主界面，当前是哪个tab

private:
    Ui::transfer *ui;
};

#endif // TRANSFER_H
