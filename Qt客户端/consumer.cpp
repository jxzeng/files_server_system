#include "consumer.h"
#include "Singleton.h"
#include "myfilewg.h"


consumer::consumer()
{

}

void consumer::run()
{
    cout<<QThread::currentThreadId()<<"正在运行"<<endl;
    while(1){
        // 获取上传列表实例
        UploadTask *uploadList = UploadTask::getInstance();
        if(uploadList == NULL)
        {
            cout << "UploadTask::getInstance() == NULL";
            return;
        }
        //请求信号量-->加锁
        uploadList->sem_data->acquire(1);
        uploadList->mutex.lock();
        // 获取登陆信息实例
        Singleton *login = Singleton::getInstance();
        myfilewg m_wg;
        // url
        m_man = new QNetworkAccessManager();
        QNetworkRequest request;
        QString url = QString("http://%1:%2/md5").arg( login->getIp() ).arg( login->getPort() );
        request.setUrl( QUrl( url )); //设置url
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        // 取出一个上传任务
        UploadFileInfo *info = uploadList->list.front();
        uploadList->list.pop_front();
        uploadList->mutex.unlock();
        uploadList->sem_empty->release(1);
        // post数据包
        QByteArray array = m_wg.setMd5Json(login->getUser(), login->getToken(), info->md5, info->fileName);
        // 发送post请求
        QNetworkReply *reply = m_man->post(request, array);
        if(reply == NULL)
        {
            cout << "reply is NULL";
            return;
        }
        connect(reply, &QNetworkReply::finished, [=]()
        {
            cout<<QThread::currentThreadId()<<"正在运行"<<endl;
            if (reply->error() != QNetworkReply::NoError) //有错误
            {
                cout << reply->errorString();
                reply->deleteLater(); //释放资源
                return;
            }
            QByteArray array = reply->readAll();
            cout<< array<<endl;
            reply->deleteLater();
            /*
                秒传文件：
                    秒传成功：  {"code":"005"}
                    秒传失败：  {"code":"006"}
                    文件已存在：{"code":"007"}
                    token验证失败：{"code":"111"}

            */
            if("005" == m_cm.getCode(array) )
            {
                //秒传文件成功
                //cout << info->fileName.toUtf8().data() << "-->秒传成功";
                m_cm.writeRecord(login->getUser(), info->fileName, "005");
                //删除已经完成的上传任务
                emit datapro(info->dp,1, 1);
                emit layout(info);

            }
            else if("006" == m_cm.getCode(array) )
            {
                // 说明后台服务器没有此文件，需要真正的文件上传
                uploadFile(info);
            }
            else if("007" == m_cm.getCode(array) )// "007", 上传的文件已存在
            {
                m_cm.writeRecord(login->getUser(), info->fileName, "007");
                //删除已经完成的上传任务
                emit datapro(info->dp,1, 1);
                emit layout(info);
            }
            else if("111" == m_cm.getCode(array)) //token验证失败
            {
                emit warm_("账户异常,请重新登陆！！！");
                //emit loginAgainSignal(); //发送重新登陆信号
                return;
            }
        });
        exec();
    }

}



void consumer::uploadFile(UploadFileInfo *info)
{
    //取出上传任务
    QFile *file = info->file;           //文件指针
    QString fileName = info->fileName;  //文件名字
    QString md5 = info->md5;            //文件md5码
    qint64 size = info->size;           //文件大小
    DataProgress *dp = info->dp;        //进度条控件
    QString boundary = m_cm.getBoundary();   //产生分隔线
    //获取登陆信息实例    
    Singleton *login = Singleton::getInstance();
    QHttpPart part;
    QString disp = QString("form-data; user=\"%1\"; filename=\"%2\"; md5=\"%3\"; size=%4")
            .arg(login->getUser()).arg(info->fileName).arg(info->md5).arg(info->size);
    part.setHeader(QNetworkRequest::ContentDispositionHeader, disp);
    part.setHeader(QNetworkRequest::ContentTypeHeader, "image/png"); // 传输的文件对应的content-type
    part.setBodyDevice(info->file);
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->append(part);    
    QNetworkRequest request; //请求对象
    QString url = QString("http://%1:%2/upload").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); //设置url
    // qt默认的请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data");
    // 发送post请求
    QNetworkReply * reply = m_man->post( request, multiPart );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    // 有可用数据更新时
    connect(reply, &QNetworkReply::uploadProgress, [=](qint64 bytesRead, qint64 totalBytes)
    {
        if(totalBytes != 0)
        {
            //cout << bytesRead/1024 << ", " << totalBytes/1024;
            //dp->setProgress(bytesRead/1024, totalBytes/1024); //设置进度条
            emit datapro(dp,bytesRead/1024, totalBytes/1024);
        }
    });
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            m_cm.writeRecord(login->getUser(), info->fileName, "009");
            //重新添加任务
            //QString path = info->path;

            emit layout(info);
            //UploadTask *uploadList = UploadTask::getInstance();
            //uploadList->appendUploadList(path);
            return;
        }
        QByteArray array = reply->readAll();
        reply->deleteLater();
        // 析构对象
        multiPart->deleteLater();

        cout<<array<<endl;
        /*
            上传文件：
                成功：{"code":"008"}
                失败：{"code":"009"}
        */
        if("008" == m_cm.getCode(array) )
        {
            //cout << fileName.toUtf8().data() <<" ---> 上传完成";
            m_cm.writeRecord(login->getUser(), info->fileName, "008");
        }
        else if("009" == m_cm.getCode(array) )
        {
            //cout << fileName.toUtf8().data() << " ---> 上传失败";
            m_cm.writeRecord(login->getUser(), info->fileName, "009");
            emit warm_(info->fileName);
        }
        emit layout(info);
    }
    );
}
