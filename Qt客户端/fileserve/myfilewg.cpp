#include "myfilewg.h"
#include "ui_myfilewg.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QListWidgetItem>
#include "Singleton.h"
#include "filepropertyinfo.h"
#include "downloadtask.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include "consumer.h"
myfilewg::myfilewg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::myfilewg)
{
    ui->setupUi(this);
    // 初始化listWidget文件列表
    initListWidget();
    // 添加右键菜单
    addActionMenu();

    // http管理类对象
    m_manager = Common::getNetManager();

    // 定时检查任务队列
    checkTaskList();
}

myfilewg::~myfilewg()
{
    delete ui;
}

void myfilewg::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);   //设置显示图标模式
    ui->listWidget->setIconSize(QSize(50, 50));         //设置图标大小
    ui->listWidget->setGridSize(QSize(70, 90));       //设置item大小
    // 设置大小改变时，图标的调整模式
    ui->listWidget->setResizeMode(QListView::Adjust);
    // 设置列表可以拖动
    ui->listWidget->setMovement(QListView::Static);
    // 设置图标之间的间距
    ui->listWidget->setSpacing(30);

    // listWidget 右键菜单
    // 发出 customContextMenuRequested 信号
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, &myfilewg::rightMenu);
    // 点中列表中的上传文件图标
    connect(ui->listWidget, &QListWidget::itemPressed, this, [=](QListWidgetItem* item)
    {
        QString str = item->text();
        if(str == "上传文件")
        {
            //添加需要上传的文件到上传任务列表
            addUploadFiles();
        }
    });
}

void myfilewg::addActionMenu()
{
    m_menu = new MyMenu( this );
    // 初始化菜单项
    m_downloadAction = new QAction("下载", this);
    m_shareAction = new QAction("分享", this);
    m_delAction = new QAction("删除", this);
    m_propertyAction = new QAction("属性", this);

    // 动作1添加到菜单1
    m_menu->addAction(m_downloadAction);
    m_menu->addAction(m_shareAction);
    m_menu->addAction(m_delAction);
    m_menu->addAction(m_propertyAction);

    //===================菜单2===================
    m_menuEmpty = new MyMenu( this );
    // 动作2
    m_pvAscendingAction = new QAction("按下载量升序", this);
    m_pvDescendingAction = new QAction("按下载量降序", this);
    m_refreshAction = new QAction("刷新", this);
    m_uploadAction = new QAction("上传", this);

    // 动作2添加到菜单2
    m_menuEmpty->addAction(m_pvAscendingAction);
    m_menuEmpty->addAction(m_pvDescendingAction);
    m_menuEmpty->addAction(m_refreshAction);
    m_menuEmpty->addAction(m_uploadAction);

    //=====================信号与槽===================
    // 下载
    connect(m_downloadAction, &QAction::triggered, [=]()
    {
        cout << "下载动作";
        //添加需要下载的文件到下载任务列表
        addDownloadFiles();
    });
    // 分享
    connect(m_shareAction, &QAction::triggered, [=]()
    {
        cout << "分享动作";
        dealSelectdFile("分享"); //处理选中的文件
    });
    // 删除
    connect(m_delAction, &QAction::triggered, [=]()
    {
        cout << "删除动作";
        dealSelectdFile("删除");
    });
    // 属性
    connect(m_propertyAction, &QAction::triggered, [=]()
    {
        cout << "属性动作";
        dealSelectdFile("属性");
    });
    // 按下载量升序
    connect(m_pvAscendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量升序";
        refreshFiles(PvAsc);
    });
    // 按下载量降序
    connect(m_pvDescendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量降序";
        refreshFiles(PvDesc);
    });
    //刷新
    connect(m_refreshAction, &QAction::triggered, [=]()
    {
        cout << "刷新动作";
        //显示用户的文件列表
        refreshFiles();
    });
    //上传
    connect(m_uploadAction, &QAction::triggered, [=]()
    {
        cout << "上传动作";
        //添加需要上传的文件到上传任务列表
        addUploadFiles();
    });
}

void myfilewg::addUploadFiles()
{    
    //获取上传类实例
    UploadTask *uploadList = UploadTask::getInstance();
    if(uploadList == nullptr)
    {
        cout << "UploadTask::getInstance() == nullptr";
        return;
    }
    QStringList list = QFileDialog::getOpenFileNames();
    for(int i = 0; i < list.size(); ++i)
    {
        //cout << "所选文件为："<<list.at(i);
        //  -1：上传的文件是否已经在上传队列中
        //  -2: 打开文件失败
        //  -3: 获取布局失败
        int res = uploadList->appendUploadList(list.at(i));
        if(res == -1)
        {
            QMessageBox::warning(this, "添加失败", "上传的文件是否已经在上传队列中！！！");
        }
        else if(res == -2)
        {
            cout << "打开文件失败";
        }
        else if(res == -3)
        {
            cout << "获取布局失败";
        }
    }
    // 切换到传输列表的, 上传界面
    emit gotoTransfer(TransferStatus::Uplaod);
}

QByteArray myfilewg::setMd5Json(QString user, QString token, QString md5, QString fileName)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
    tmp.insert("md5", md5);
    tmp.insert("fileName", fileName);
    /*json数据如下
    {
        user:xxxx,
        token:xxxx,
        md5:xxx,
        fileName: xxx
    }
    */
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    //cout << jsonDocument.toJson().data();
    return jsonDocument.toJson();
}





void myfilewg::clearFileList()
{
    int n = m_fileList.size();
    for(int i = 0; i < n; ++i)
    {
        FileInfo *tmp = m_fileList.takeFirst();
        delete tmp;
    }
}

void myfilewg::clearItems()
{
    //使用QListWidget::count()来统计ListWidget中总共的item数目
    int n = ui->listWidget->count();
    for(int i = 0; i < n; ++i)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0); //这里是0，不是i
        delete item;
    }
}

void myfilewg::addUploadItem(QString iconPath, QString name)
{
    ui->listWidget->addItem(new QListWidgetItem(QIcon(iconPath), name));
}

void myfilewg::refreshFileItems()
{
    //清空所有item项目
    clearItems();
    //如果文件列表不为空，显示文件列表
    if(m_fileList.isEmpty() == false)
    {
        int n = m_fileList.size(); //元素个数
        for(int i = 0; i < n; ++i)
        {
            FileInfo *tmp = m_fileList.at(i);
            QListWidgetItem *item = tmp->item;
            ui->listWidget->addItem(item);
        }
    }
    this->addUploadItem();
}

QStringList myfilewg::getCountStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;
    //将来源数据json转化为JsonDocument
    //由QByteArray对象构造一个QJsonDocument对象，用于我们的读写操作
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error == QJsonParseError::NoError) //没有出错
    {
        if (doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return list;
        }
        if( doc.isObject() )
        {
            QJsonObject obj = doc.object();//取得最外层这个大对象
            list.append( obj.value( "token" ).toString() ); //登陆token
            list.append( obj.value( "num" ).toString() ); //文件个数
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }

    return list;
}

void myfilewg::refreshFiles(myfilewg::Display cmd)
{
    //=========================>先获取用户文件数目<=========================
    m_userFilesCount = 0;
    QNetworkRequest request; //请求对象
    // 获取登陆信息实例
    Singleton *login = Singleton::getInstance();
    // 127.0.0.1:80/myfiles?cmd=count		//获取用户文件个数
    QString url = QString("http://%1:%2/myfiles?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); //设置url
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QByteArray data = setGetCountJson( login->getUser(), login->getToken());

    // 发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }
        // 服务器返回数据
        QByteArray array = reply->readAll();
        //cout << "server return file ...: " << array;
        reply->deleteLater(); //释放
        // 得到服务器json文件
        QStringList list = getCountStatus(array);
        // token验证失败
        if( list.at(0) == "111" )
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号
            return;
        }
        // 转换为long
        m_userFilesCount = list.at(1).toLong();
        //cout << "userFilesCount = " << m_userFilesCount;
        // 清空文件列表信息
        clearFileList();
        if(m_userFilesCount > 0)
        {
            // 说明用户有文件，获取用户文件列表
            m_start = 0;  //从0开始
            m_count = 10; //每次请求10个
            // 获取新的文件列表信息
            getUserFilesList(cmd);
        }
        else //没有文件
        {
            refreshFileItems(); //更新item
        }
    });
}

QByteArray myfilewg::setGetCountJson(QString user, QString token)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);

    /*json数据如下
    {
        user:xxxx
        token: xxxx
    }
    */
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();
}

QByteArray myfilewg::setFilesListJson(QString user, QString token, int start, int count)
{
    /*{
        "user": "yoyo"
        "token": "xxx"
        "start": 0
        "count": 10
    }*/
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
    tmp.insert("start", start);
    tmp.insert("count", count);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();
}

void myfilewg::getUserFilesList(myfilewg::Display cmd)
{
    //遍历数目，结束条件处理
    if(m_userFilesCount <= 0)
    {
        cout << "获取用户文件列表条件结束";
        refreshFileItems(); //更新item
        return; //中断函数
    }
    else if(m_count > m_userFilesCount) //如果请求文件数量大于用户的文件数目
    {
        m_count = m_userFilesCount;
    }

    QNetworkRequest request; //请求对象
    // 获取登陆信息实例
    Singleton *login = Singleton::getInstance();
    // 获取用户文件信息 127.0.0.1:80/myfiles&cmd=normal
    // 按下载量升序 127.0.0.1:80/myfiles?cmd=pvasc
    // 按下载量降序127.0.0.1:80/myfiles?cmd=pvdesc
    QString url;
    QString tmp;
    //cmd取值，Normal：普通用户列表，PvAsc：按下载量升序， PvDesc：按下载量降序
    if(cmd == Normal)
    {
        tmp = "normal";
    }
    else if(cmd == PvAsc)
    {
        tmp = "pvasc";
    }
    else if(cmd == PvDesc)
    {
        tmp = "pvdesc";
    }

    url = QString("http://%1:%2/myfiles?cmd=%3").arg(login->getIp()).arg(login->getPort()).arg(tmp);
    request.setUrl(QUrl( url )); //设置url
    cout << "myfiles url: " << url;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    /*
    {
        "user": "yoyo"
        "token": "xxxx"
        "start": 0
        "count": 10
    }
    */
    QByteArray data = setFilesListJson( login->getUser(), login->getToken(), m_start, m_count);
    //改变文件起点位置
    m_start += m_count;
    m_userFilesCount -= m_count;

    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    //获取请求的数据完成时，就会发送信号SIGNAL(finished())
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }
        //服务器返回用户的数据
        QByteArray array = reply->readAll();
        cout << "file info:" << array;
        reply->deleteLater();
        //token验证失败
        if("111" == m_cm.getCode(array) ) //common.h
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal();
            return;
        }
        //不是错误码就处理文件列表json信息
        if("015" != m_cm.getCode(array) )
        {
            getFileJsonInfo(array);//解析文件列表json信息，存放在文件列表中
            //继续获取用户文件列表
            getUserFilesList(cmd);
        }
    });
}

void myfilewg::getFileJsonInfo(QByteArray data)
{
    QJsonParseError error;
    //将来源数据json转化为JsonDocument
    //由QByteArray对象构造一个QJsonDocument对象，用于我们的读写操作
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error == QJsonParseError::NoError) //没有出错
    {
        if (doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return;
        }

        if( doc.isObject())
        {
            //QJsonObject json对象，描述json数据中{}括起来部分
            QJsonObject obj = doc.object();//取得最外层这个大对象

            //获取games所对应的数组
            //QJsonArray json数组，描述json数据中[]括起来部分
            QJsonArray array = obj.value("files").toArray();

            int size = array.size();   //数组个数
            cout << "size = " << size;

            for(int i = 0; i < size; ++i)
            {
                QJsonObject tmp = array[i].toObject(); //取第i个对象
                FileInfo *info = new FileInfo;
                info->user = tmp.value("user").toString(); //用户
                info->md5 = tmp.value("md5").toString(); //文件md5
                info->time = tmp.value("time").toString(); //上传时间
                info->filename = tmp.value("filename").toString(); //文件名字
                info->shareStatus = tmp.value("share_status").toInt(); //共享状态
                info->pv = tmp.value("pv").toInt(); //下载量
                info->url = tmp.value("url").toString(); //url
                info->size = tmp.value("size").toInt(); //文件大小，以字节为单位
                info->type = tmp.value("type").toString();//文件后缀
                QString type = info->type + ".png";
                info->item = new QListWidgetItem(QIcon( m_cm.getFileType(type) ), info->filename);
                //list添加节点
                m_fileList.append(info);
            }
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }
}

void myfilewg::dealSelectdFile(QString cmd)
{
    //获取当前选中的item
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        return;
    }
    //查找文件列表匹配的元素
    for(int i = 0; i < m_fileList.size(); ++i)
    {
        if(m_fileList.at(i)->item == item)
        {
            if(cmd == "分享")
            {
                shareFile( m_fileList.at(i) ); //分享某个文件
            }
            else if(cmd == "删除")
            {
                delFile( m_fileList.at(i) ); //删除某个文件
            }
            else if(cmd == "属性")
            {
                getFileProperty( m_fileList.at(i) ); //获取属性信息
            }
            break;
        }
    }
}

QByteArray myfilewg::setDealFileJson(QString user, QString token, QString md5, QString filename)
{
    /*
    {
        "user": "yoyo",
        "token": "xxxx",
        "md5": "xxx",
        "filename": "xxx"
    }
    */
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
    tmp.insert("md5", md5);
    tmp.insert("filename", filename);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    //cout << jsonDocument.toJson().data();

    return jsonDocument.toJson();
}

void myfilewg::shareFile(FileInfo *info)
{
    if(info->shareStatus == 1)
    {
        QMessageBox::warning(this, "此文件已经分享", "此文件已经分享!!!");
        return;
    }
    QNetworkRequest request; //请求对象
    //获取登陆信息实例
    Singleton *login = Singleton::getInstance();
    //127.0.0.1:80/dealfile?cmd=share
    QString url = QString("http://%1:%2/dealfile?cmd=share").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); //设置url
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    /*
    {
        "user": "yoyo",
        "token": "xxxx",
        "md5": "xxx",
        "filename": "xxx"
    }
    */
    QByteArray data = setDealFileJson( login->getUser(),  login->getToken(), info->md5, info->filename);

    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    //获取请求的数据完成时，就会发送信号SIGNAL(finished())
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }

        //服务器返回用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();
        /*
            分享文件：
                成功：{"code":"010"}
                失败：{"code":"011"}
                别人已经分享此文件：{"code", "012"}

            token验证失败：{"code":"111"}

        */
        if("010" == m_cm.getCode(array) ) //common.h
        {
            //修改文件列表的属性信息
            info->shareStatus = 1;
            QMessageBox::information(this, "分享成功", QString("[%1] 分享成功!!!").arg(info->filename));
        }
        else if("011" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "分享失败", QString("[%1] 分享失败!!!").arg(info->filename));
        }
        else if("012" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "分享失败", QString("[%1] 别人已分享此文件!!!").arg(info->filename));
        }
        else if("111" == m_cm.getCode(array)) //token验证失败
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal();
            return;
        }
    });
}

void myfilewg::delFile(FileInfo *info)
{
    QNetworkRequest request;
    Singleton *login = Singleton::getInstance();
    //127.0.0.1:80/dealfile?cmd=del
    QString url = QString("http://%1:%2/dealfile?cmd=del").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    /*
        {
            "user": "yoyo",
            "token": "xxxx",
            "md5": "xxx",
            "filename": "xxx"
        }
    */
    QByteArray data = setDealFileJson( login->getUser(),  login->getToken(), info->md5, info->filename);

    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    //获取请求的数据完成时，就会发送信号SIGNAL(finished())
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }

        //服务器返回用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();
        /*
            删除文件：
                成功：{"code":"013"}
                失败：{"code":"014"}
        */
        if("013" == m_cm.getCode(array) )
        {
            QMessageBox::information(this, "文件删除成功", QString("[%1] 删除成功!!!").arg(info->filename));
            //从文件列表中移除该文件，移除列表视图中此item
            for(int i = 0; i < m_fileList.size(); ++i)
            {
                if( m_fileList.at(i) == info)
                {
                    QListWidgetItem *item = info->item;
                    //从列表视图移除此item
                    ui->listWidget->removeItemWidget(item);
                    delete item;
                    m_fileList.removeAt(i);
                    delete info;
                    break;
                }
            }
        }
        else if("014" == m_cm.getCode(array))
        {
            QMessageBox::information(this, "文件删除失败", QString("[%1] 删除失败!!!").arg(info->filename));
        }
        else if("111" == m_cm.getCode(array)) //token验证失败
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal();
            return;
        }
    });
}

void myfilewg::getFileProperty(FileInfo *info)
{
    FilePropertyInfo dlg; //创建对话框
    dlg.setInfo(info);
    dlg.exec(); //模态方式运行
}

void myfilewg::addDownloadFiles()
{
    emit gotoTransfer(TransferStatus::Download);
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        cout << "item == NULL";
        return;
    }
    //获取下载列表实例
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    for(int i = 0; i < m_fileList.size(); ++i)
    {
        if(m_fileList.at(i)->item == item)
        {

            QString filePathName = QFileDialog::getSaveFileName(this, "选择保存文件路径", m_fileList.at(i)->filename );
            if(filePathName.isEmpty())
            {
                cout << "filePathName.isEmpty()";
                return;
            }

            /*
               下载文件：
                    成功：{"code":"009"}
                    失败：{"code":"010"}
            */
            //cout << filePathName;

            //追加任务到下载队列
            //参数：info：下载文件信息， filePathName：文件保存路径
            //成功：0
            //失败：
            //  -1: 下载的文件是否已经在下载队列中
            //  -2: 打开文件失败
            int res = p->appendDownloadList(m_fileList.at(i), filePathName); //追加到下载列表
            if(res == -1)
            {
                QMessageBox::warning(this, "任务已存在", "任务已经在下载队列中！！！");
            }
            else if(res == -2) //打开文件失败
            {
                m_cm.writeRecord(m_fileList.at(i)->user, m_fileList.at(i)->filename, "010"); //下载文件失败，记录
            }
            break;
        }
    }
}

void myfilewg::downloadFilesAction()
{
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    if( p->isEmpty() ) //如果队列为空，说明没有任务
    {
        return;
    }

    if( p->isDownload() ) //当前时间没有任务在下载，才能下载，单任务
    {
        return;
    }

    //看是否是共享文件下载任务，不是才能往下执行, 如果是，则中断程序
    if(p->isShareTask() == true)
    {
        return;
    }

    DownloadInfo *tmp = p->takeTask(); //取下载任务
    QUrl url = tmp->url;
    QFile *file = tmp->file;
    QString md5 = tmp->md5;
    QString user = tmp->user;
    QString filename = tmp->filename;
    DataProgress *dp = tmp->dp;
    //发送get请求
    QNetworkReply * reply = m_manager->get( QNetworkRequest(url) );
    if(reply == NULL)
    {
        p->dealDownloadTask(); //删除任务
        cout << "get err";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        cout << "下载完成";
        reply->deleteLater();
        p->dealDownloadTask();//删除下载任务
        m_cm.writeRecord(user, filename, "010"); //下载文件成功，记录
        dealFilePv(md5, filename); //下载文件pv字段处理
    });
    //当有可用数据时，reply 就会发出readyRead()信号，我们这时就可以将可用的数据保存下来
    connect(reply, &QNetworkReply::readyRead, [=]()
    {
        //如果文件存在，则写入文件
        if (file != NULL)
        {
            file->write(reply->readAll());
        }
    });
    //有可用数据更新时
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesRead, qint64 totalBytes)
    {
        dp->setProgress(bytesRead, totalBytes);//设置进度
    }
    );
}

void myfilewg::dealFilePv(QString md5, QString filename)
{
    QNetworkRequest request;
    Singleton *login = Singleton::getInstance();
    //127.0.0.1:80/dealfile?cmd=pv
    QString url = QString("http://%1:%2/dealfile?cmd=pv").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); //设置url
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    /*
    {
        "user": "yoyo",
        "token": "xxx",
        "md5": "xxx",
        "filename": "xxx"
    }
    */
    QByteArray data = setDealFileJson( login->getUser(), login->getToken(), md5, filename);
    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }

        //服务器返回用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();
        /*
            下载文件pv字段处理
                成功：{"code":"016"}
                失败：{"code":"017"}
            */
        if("016" == m_cm.getCode(array) ) //common.h
        {
            //该文件pv字段+1
            for(int i = 0; i < m_fileList.size(); ++i)
            {
                FileInfo *info = m_fileList.at(i);
                if( info->md5 == md5 && info->filename == filename)
                {
                    int pv = info->pv;
                    info->pv = pv+1;
                    break;
                }
            }
        }
        else
        {
            cout << "下载文件pv字段处理失败";
        }
    });
}

void myfilewg::clearAllTask()
{
    //获取上传列表实例
    UploadTask *uploadList = UploadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }
    uploadList->clearList();
    //获取下载列表实例
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }
    p->clearList();
}

void myfilewg::checkTaskList()
{    

    // 定时检查下载队列是否有任务需要下载
    connect(&m_downloadTimer, &QTimer::timeout, [=]()
    {
        // 下载文件处理，取出下载任务列表的队首任务，下载完后，再取下一个任务
        downloadFilesAction();
    });
    // 启动定时器，500毫秒间隔
    // 每个500毫秒，检测下载任务，每一次只能下载一个文件
    m_downloadTimer.start(500);
}

void myfilewg::contextMenuEvent(QContextMenuEvent *event)
{

}

void myfilewg::rightMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget->itemAt( pos );
    if( item == NULL ) //没有点图标
    {
        m_menuEmpty->exec( QCursor::pos() ); //在鼠标点击的地方弹出菜单
    }
    else //点图标
    {
        ui->listWidget->setCurrentItem(item);
        if(item->text() == "上传文件") //如果是上传文件，没有右击菜单
        {
            return;
        }
        m_menu->exec( QCursor::pos() );
    }
}
