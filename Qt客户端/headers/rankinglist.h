#ifndef RANKINGLIST_H
#define RANKINGLIST_H

#include <QWidget>
#include "common.h"
namespace Ui {
class rankinglist;
}

// 文件信息
struct RankingFileInfo
{
    QString filename;   // 文件名字
    int pv;             // 下载量
};

class rankinglist : public QWidget
{
    Q_OBJECT

public:
    explicit rankinglist(QWidget *parent = nullptr);
    ~rankinglist();

    // 设置TableWidget表头和一些属性
    void initTableWidget();

    // 清空文件列表
    void clearshareFileList();

    // ==========>显示共享文件列表<==============
    void refreshFiles();                                // 显示共享的文件列表
    QByteArray setFilesListJson(int start, int count);  // 设置json包
    void getUserFilesList();                            // 获取共享文件列表
    void getFileJsonInfo(QByteArray data);              // 解析文件列表json信息，存放在文件列表中

    // 更新排行版列表
    void refreshList();
private:
    Ui::rankinglist *ui;
    Common m_cm;
    QNetworkAccessManager *m_manager;

    int  m_start;                      // 文件位置起点
    int  m_count;                      // 每次请求文件个数
    long m_userFilesCount;             // 用户文件数目
    QList<RankingFileInfo *> m_list;   // 文件列表
};

#endif // RANKINGLIST_H
