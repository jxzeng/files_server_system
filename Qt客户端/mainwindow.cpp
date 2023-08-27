#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Singleton.h"
#include "consumer.h"
#include "uploadlayout.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    
    //去掉边框
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    ui->close_widget->setParent(this);
    // 处理所有信号
    managerSignals();    
    // 默认显示我的文件窗口
    ui->stackedWidget->setCurrentWidget(ui->myfiles_page);
    connect(this, &MainWindow::setUser, ui->top_widget, &main_topwidget::changeCurrUser);

    csumer_1.start();
    csumer_2.start();
    csumer_3.start();
    connect(&csumer_1, &consumer::warm_,this, [=]()
    {
        qDebug() << "账户异常,请重新登陆！！！";
        loginAgain();
        csumer_1.quit();
    });
    connect(&csumer_2, &consumer::warm_,this, [=]()
    {
        qDebug() << "账户异常,请重新登陆！！！";
        loginAgain();
        csumer_2.quit();
    });
    connect(&csumer_3, &consumer::warm_,this, [=]()
    {
        qDebug() << "账户异常,请重新登陆！！！";
        loginAgain();
        csumer_3.quit();
    });
    connect(&csumer_1, &consumer::datapro,this, [=](DataProgress* dp,int value, int max)
    {
        dp->setProgress(value, max);
    });
    connect(&csumer_2, &consumer::datapro,this, [=](DataProgress* dp,int value, int max)
    {
        dp->setProgress(value, max);
    });
    connect(&csumer_3, &consumer::datapro,this, [=](DataProgress* dp,int value, int max)
    {
        dp->setProgress(value, max);
    });
    connect(&csumer_1, &consumer::layout,this, [=](UploadFileInfo *tmp)
    {
        //获取布局
        UploadLayout *pUpload = UploadLayout::getInstance();
        QLayout *layout = pUpload->getUploadLayout();
        layout->removeWidget(tmp->dp); //从布局中移除控件
        //关闭打开的文件指针
        QFile *file = tmp->file;
        file->close();
        delete file;
        delete tmp->dp;
        delete tmp; //释放空间
        csumer_1.quit();
    });
    connect(&csumer_2, &consumer::layout,this, [=](UploadFileInfo *tmp)
    {
        //获取布局
        UploadLayout *pUpload = UploadLayout::getInstance();
        QLayout *layout = pUpload->getUploadLayout();
        layout->removeWidget(tmp->dp); //从布局中移除控件
        //关闭打开的文件指针
        QFile *file = tmp->file;
        file->close();
        delete file;
        delete tmp->dp;
        delete tmp; //释放空间
        csumer_2.quit();
    });
    connect(&csumer_3, &consumer::layout,this, [=](UploadFileInfo *tmp)
    {
        //获取布局
        UploadLayout *pUpload = UploadLayout::getInstance();
        QLayout *layout = pUpload->getUploadLayout();
        layout->removeWidget(tmp->dp); //从布局中移除控件
        //关闭打开的文件指针
        QFile *file = tmp->file;
        file->close();
        delete file;
        delete tmp->dp;
        delete tmp; //释放空间
        csumer_3.quit();
    });
}

MainWindow::~MainWindow()
{
    csumer_1.quit();
    csumer_1.wait();
    csumer_2.quit();
    csumer_2.wait();
    csumer_3.quit();
    csumer_3.wait();
    delete ui;
}
void MainWindow::dealsignal(QString str)
{
    cout<<"nihao\t"<<str<<endl;
}
void MainWindow::showMainWindow()
{
    m_common.moveToCenter(this);
    // 切换到我的文件页面
    ui->stackedWidget->setCurrentWidget(ui->myfiles_page);
    // 刷新用户文件列表
    ui->myfiles_page->refreshFiles();
    // 发送信号，告诉登陆窗口，初始化当前登录用户
    Singleton* login = Singleton::getInstance();
    emit setUser(login->getUser());
}

void MainWindow::managerSignals()
{

    // 我的文件
    connect(ui->right_widget, &mainwindows_rightwidget::sigMyFile, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->myfiles_page);
        // 刷新文件列表
        ui->myfiles_page->refreshFiles();
    });
#if 1
    // 分享列表
    connect(ui->right_widget, &mainwindows_rightwidget::sigShareList, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->sharefile_page);
        // 刷新分享列表
        ui->sharefile_page->refreshFiles();
    });
    // 下载榜
    connect(ui->right_widget, &mainwindows_rightwidget::sigDownload, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->ranking_page);
        // 刷新下载榜列表
        ui->ranking_page->refreshFiles();
    });
    // 传输列表
    connect(ui->right_widget, &mainwindows_rightwidget::sigTransform, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->transfer_page);
    });
    // 切换用户
    connect(ui->right_widget, &mainwindows_rightwidget::sigSwitchUser, [=]()
    {
        qDebug() << "bye bye...";
        loginAgain();
    });

    // stack窗口切换
    connect(ui->myfiles_page, &myfilewg::gotoTransfer, [=](TransferStatus status)
    {
        ui->right_widget->slotButtonClick(Page::TRANSFER);
        if(status == TransferStatus::Uplaod)
        {
            ui->transfer_page->showUpload();
        }
        else if(status == TransferStatus::Download)
        {
            ui->transfer_page->showDownload();
        }
    });
    // 信号传递
    connect(ui->sharefile_page, &sharelist::gotoTransfer, ui->myfiles_page, &myfilewg::gotoTransfer);
#endif
}
void MainWindow::loginAgain()
{
    // 发送信号，告诉登陆窗口，切换用户
    emit changeUser();
    // 清空上一个用户的上传或下载任务
    ui->myfiles_page->clearAllTask();
    // 清空上一个用户的一些文件显示信息
    ui->myfiles_page->clearFileList();
    ui->myfiles_page->clearItems();
}



