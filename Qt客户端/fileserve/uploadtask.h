#ifndef UPLOADTASK_H
#define UPLOADTASK_H

#include "common.h"
#include <QVBoxLayout>
#include <QFile>
#include "dataprogress.h"
#include <QThread>
#include <QSemaphore>
#include <QMutex>
//上传文件信息
struct UploadFileInfo
{
    QString md5;        //文件md5码
    QFile *file;        //文件指针
    QString fileName;   //文件名字
    qint64 size;        //文件大小
    QString path;       //文件路径
    bool isUpload;      //是否已经在上传
    DataProgress *dp;   //上传进度控件
};

//上传任务列表类，单例模式
class UploadTask
{
public:
    static UploadTask *getInstance(); //保证唯一一个实例    
    int appendUploadList(QString path);
    bool isEmpty(); //判断上传队列释放为空
    bool isUpload(); //是否有文件正在上传

    //取出第0个上传任务，如果任务队列没有任务在上传，设置第0个任务上传
    UploadFileInfo * takeTask();
    //删除上传完成的任务
    void dealUploadTask();

    void clearList(); //清空上传列表

private:
    UploadTask();    //构造函数为私有

    ~UploadTask()    //析构函数为私有
    {
    }

    //静态数据成员，类中声明，类外必须定义
    static UploadTask *instance;

    //它的唯一工作就是在析构函数中删除Singleton的实例
    class Garbo
    {
    public:
        ~Garbo()
        {
          if(NULL != UploadTask::instance)
          {
            UploadTask::instance->clearList();

            delete UploadTask::instance;
            UploadTask::instance = NULL;
            cout << "instance is detele";
          }
        }
    };
    static Garbo temp; //静态数据成员，类中声明，类外定义

public:
    QList<UploadFileInfo *> list; //任务队列    
    QMutex mutex; //互斥锁
    QSemaphore *sem_empty; //临界区剩余的数
    QSemaphore *sem_data;  //临界区生产的数

};

#endif // UPLOADTASK_H
