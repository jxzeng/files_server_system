#include "login.h"
#include "ui_login.h"

login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    //去掉边框
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    //把窗口背景设置为透明;
    this->setAttribute(Qt::WA_TranslucentBackground);

    //密码隐藏
    ui->lineEdit_pw->setEchoMode(QLineEdit::Password);
    ui->reg_password->setEchoMode(QLineEdit::Password);
    ui->reg_queren->setEchoMode(QLineEdit::Password);
    m_parent = parent;
    // 当前显示的窗口
    ui->stackedWidget->setCurrentIndex(0);
    ui->lineEdit_id->setFocus();
    // 数据的格式提示
    //ui->lineEdit_id->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 3~16");
    ui->reg_username->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 3~16");
    ui->reg_nicheng->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 3~16");
    ui->lineEdit_pw->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 6~18");
    ui->reg_password->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 6~18");
    ui->reg_queren->setToolTip("合法字符:字母、数字、特殊符号(#@-_*),个数: 6~18");
    // 读取配置文件信息
    fill_info();
    //事件实现
    connect(ui->set,&QToolButton::clicked,[=](){
        ui->stackedWidget->setCurrentWidget(ui->set_page);
    });
    connect(ui->min,&QToolButton::clicked,[=](){
        this->showMinimized();
    });
    connect(ui->close,&QToolButton::clicked,[=](){
        if(ui->stackedWidget->currentWidget() == ui->set_page){
            //切换回登录界面
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            //
        }
        else if(ui->stackedWidget->currentWidget() == ui->reg_page){
            //切换回登录界面
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            //需要清除数据
            ui->reg_password->clear();
            ui->reg_email->clear();
            ui->reg_phone->clear();
            ui->reg_queren->clear();
            ui->reg_nicheng->clear();
            ui->reg_username->clear();


        }
        else{
            this->close();
        }
    });
    // 切换用户 - 重新登录
    connect(&m_mainWin, &MainWindow::changeUser, [=]()
    {
        m_mainWin.hide();
        this->show();
    });
}

login::~login()
{
    delete ui;
}

void login::mouseMoveEvent(QMouseEvent *event)
{
    //只允许左键拖动
    if(event->buttons() & Qt::LeftButton){
        //窗口跟随鼠标移动
        move(event->globalPos()-m_pt);
    }
}

void login::mousePressEvent(QMouseEvent *ev)
{
    //如果鼠标左键按下
    if(ev->button() == Qt::LeftButton){
        m_pt = ev->globalPos()-this->geometry().topLeft();
    }
}


void login::on_Registration_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->reg_page);
}

void login::on_pushButton_2_clicked()
{
    /*
        流程：1、获取控件text
              2、数据校验
              3、发送数据—》服务器
              4、接收服务器响应信息
              5、解析数据，分析是否注册成功
    */
    //获取控件数据
    QString username = ui->reg_username->text();
    QString nickname = ui->reg_nicheng->text();
    QString password = ui->reg_password->text();
    QString confirmpw = ui->reg_queren->text();
    QString phone = ui->reg_phone->text();
    QString email = ui->reg_email->text();
#if 0
    //数据初步校验，如Email格式、密码、确认密码是否相同
    //username校验
    QRegExp exp(USER_REG);
    if(!exp.exactMatch(username)){
        QMessageBox::warning(this,"Note","用户名格式不正确！！");
        ui->reg_username->clear();
        ui->reg_username->setFocus();
        return;
    }
    //nickname校验
    if(!exp.exactMatch(nickname)){
        QMessageBox::warning(this,"Note","昵称格式不正确！！");
        ui->reg_nicheng->clear();
        ui->reg_nicheng->setFocus();
        return;
    }

    exp.setPattern(PASSWD_REG);
    if(!exp.exactMatch(password)){
        QMessageBox::warning(this,"Note","密码格式不正确！！");
        ui->reg_password->clear();
        ui->reg_password->setFocus();
        return;
    }
    if(password != confirmpw)
        {
            QMessageBox::warning(this, "警告", "两次输入的密码不匹配, 请重新输入");
            ui->reg_queren->clear();
            ui->reg_queren->setFocus();
            return;
        }
    exp.setPattern(PHONE_REG);
    if(!exp.exactMatch(phone)){
        QMessageBox::warning(this,"Note","电话格式不正确！！");
        ui->reg_phone->clear();
        ui->reg_phone->setFocus();
        return;
    }
    exp.setPattern(EMAIL_REG);
    if(!exp.exactMatch(email)){
        QMessageBox::warning(this,"Note","邮箱格式不正确！！");
        ui->reg_email->clear();
        ui->reg_email->setFocus();
        return;
    }
#endif
    cout<<ui->setpage_svip->text()<<endl<<ui->setpage_sport->text();
    /*
        网络通信
        使用http协议，使用post方式
        数据格式采用json对象
    */
    //将用户提交数据拼接成JSon对象字符串
    QJsonObject obj;
    obj.insert("userName",username);
    obj.insert("nickName",nickname);
    obj.insert("firstPwd",m_cm.getStrMd5(password));
    obj.insert("confirmpw",m_cm.getStrMd5(confirmpw));
    obj.insert("phone",phone);
    obj.insert("email",email);
    QJsonDocument doc(obj);
    //cout<<doc<<endl;
    QByteArray data = doc.toJson();
    QNetworkAccessManager* pManager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    QString url = QString("http://%1:%2/reg").arg(ui->setpage_svip->text()).arg(ui->setpage_sport->text());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json"); //post数据的格式    
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(data.size()));
    QNetworkReply* reply = pManager->post(request,data);
    cout<<data;
    //接收服务器发送的响应数据
    connect(reply,&QNetworkReply::readyRead,this,[=](){        
        //对服务器响应数据进行分析，successful or failure
        //接收数据
        QByteArray all = reply->readAll();
        //解析数据->jsond->jsonobj
        /*
            server端返回的json格式数据：
                成功:         {"code":"002"}
                该用户已存在：  {"code":"003"}
                失败:         {"code":"004"}
        */
        QJsonDocument reply_doc = QJsonDocument::fromJson(all);
        //cout<<reply_doc<<endl;
        if(reply_doc.isObject()){
            QJsonObject reply_obj = reply_doc.object();
            QString status = reply_obj.value("code").toString();
            if(status == "002"){
                //注册成功
                QMessageBox::information(this, "注册成功", "注册成功，请登录");
                //清空行编辑内容
                ui->reg_email->clear();
                ui->reg_phone->clear();
                ui->reg_queren->clear();
                ui->reg_nicheng->clear();
                ui->reg_password->clear();
                ui->reg_username->clear();
                //设置登陆窗口的登陆信息
                ui->lineEdit_id->setText(username);
                ui->lineEdit_pw->setText(password);
                ui->checkBox_know->setChecked(true);
                //切换到登录界面
                ui->stackedWidget->setCurrentWidget(ui->login_page);
            }
            else if(status == "003"){
                //用户已经存在
                QMessageBox::warning(this, "注册失败", QString("[%1]该用户已经存在!!!").arg(username));                
            }
            else{
                //失败
                QMessageBox::warning(this,"Note","注册失败！！");                
            }
            delete reply;
        }
    });    
}
//服务器设置
void login::on_pushButton_clicked()
{
    //获取控件数据
    QString ip = ui->setpage_svip->text();
    QString port = ui->setpage_sport->text();
    //数据校验-正则表达式
    //IP校验
    QRegExp exp(IP_REG);
    if(!exp.exactMatch(ip)){
        QMessageBox::warning(this,"Note","输入IP格式不正确！！");
        ui->setpage_svip->clear();
        ui->setpage_svip->setFocus();
        return;
    }
    //端口校验
    exp.setPattern(PORT_REG);
    if(!exp.exactMatch(port)){
        QMessageBox::warning(this,"Note","输入Port格式不正确！！");
        ui->setpage_sport->clear();
        ui->setpage_sport->setFocus();
        return;
    }
#if 0
    //保存到客户端配置文件/Json文件
    QString path = "./cfg.json";
    saveWebinfo(ip,port,path);
#endif
    //切换窗口
    ui->stackedWidget->setCurrentWidget(ui->login_page);
}
void login::saveWebinfo(QString ipc, QString portc, QString path)
{
#if 0
    //写入json文件
    QJsonObject obj;
    QJsonObject sub;
    sub.insert("IP",QJsonValue("192.168.1.2"));
    sub.insert("port","9990");
    obj.insert("server",QJsonValue(sub));
    QJsonDocument jsonD(obj);
    QByteArray data =jsonD.toJson();
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)==false){
        cout<<"open file failure";
    }
    file.write(data);
    file.close();

    //读取json文件
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isObject()){
        QJsonObject obj = doc.object();
        QJsonValue value = obj.value("server");
        if(value.isObject()){
            QJsonObject subobj = value.toObject();
            QString ip = subobj.value("IP").toString();
            QString port = subobj.value("port").toString();
            cout<<ip<<"\n"<<port;
        }
    }
#endif

}
QByteArray login::setLoginJson(QString user, QString pwd)
{
    QMap<QString, QVariant> login;
    login.insert("user", user);
    login.insert("pwd", pwd);
    /*json数据如下
        {
            user:xxxx,
            pwd:xxx
        }
    */
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(login);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }

    return jsonDocument.toJson();
}
//登录响应数据解析
QStringList login::getLoginStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error == QJsonParseError::NoError)
    {
        if (doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return list;
        }
        if( doc.isObject() )
        {
            //取得最外层这个大对象
            QJsonObject obj = doc.object();
            cout << "服务器返回的数据" << obj;
            //状态码
            list.append( obj.value( "code" ).toString());
            //登陆token
            list.append( obj.value( "token" ).toString());
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }

    return list;
}
void login::on_pushButton_ok_clicked()
{
    // 获取控件信息
    QString user = ui->lineEdit_id->text();
    QString pwd = ui->lineEdit_pw->text();
    QString ip_address = ui->setpage_svip->text();
    QString port = ui->setpage_sport->text();
    //数据校验
    QRegExp regexp(PASSWD_REG);
    if(!regexp.exactMatch(pwd))
    {
        QMessageBox::warning(this, "警告", "密码格式不正确");
        ui->lineEdit_pw->clear();
        ui->lineEdit_pw->setFocus();
        return;
    }
    //查询登录数据
    //为确保安全性，与服务端的密码数据经过md5加密
    QByteArray array = setLoginJson(user, m_cm.getStrMd5(pwd));
    //网络通信
    QNetworkAccessManager* pManager = new QNetworkAccessManager(this);
    // 设置登录的url
    QNetworkRequest request;
    QString url = QString("http://%1:%2/login").arg(ip_address).arg(port);
    request.setUrl(QUrl(url));
    // 请求头信息
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(array.size()));
    // 向服务器发送post请求
    QNetworkReply* reply = pManager->post(request, array);
    // 接收服务器发回的http响应消息
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            //释放资源
            reply->deleteLater();
            return;
        }
        //解析服务端响应数据
        QByteArray json = reply->readAll();
        /*
            登陆 - 服务器回写的json数据包格式：
                成功：{"code":"000"}
                失败：{"code":"001"}
        */
        cout << "server return value: " << json;
        QStringList tmpList = getLoginStatus(json);
        if( tmpList.at(0) == "000" )
        {
            //密码正确，如果记住密码时，保存数据，需要先加密再保存
            m_cm.writeLoginInfo(user, pwd,ip_address,port, ui->checkBox_know->isChecked());
            cout << "登陆成功";
            //设置登陆信息，显示文件列表界面需要使用这些信息
            Singleton *p = Singleton::getInstance();
            p->setLoginInfo(user, ip_address, port, tmpList.at(1));
            //cout << p->getUser().toUtf8().data() << ", " << p->getIp() << ", " << p->getPort() << tmpList.at(1);
            // 当前窗口隐藏
            this->hide();
            // 主界面窗口显示
            m_mainWin.showMainWindow();
        }
        else
        {
            QMessageBox::warning(this, "登录失败", "用户名或密码不正确！！！");
        }

        reply->deleteLater(); //释放资源
    });

}
//打开软件时读取配置信息初始化界面
void login::fill_info()
{
    //服务端信息
    QString sv_ip = m_cm.getCfgValue("web_server","ip");
    QString sv_port = m_cm.getCfgValue("web_server","port");
    //login 信息
    QString lg_user = m_cm.getCfgValue("login","user");
    QString lg_pwd = m_cm.getCfgValue("login","pwd");
    QString lg_remenber = m_cm.getCfgValue("login","remember");
    int ret = 0;
    //填充信息 user数据需要解密
    unsigned char encUsr[512] = {0};
    int encUsrLen = 0;
    //toLocal8Bit(), 转换为本地字符集，如果windows则为gbk编码，如果linux则为utf-8编码
    QByteArray tmp = QByteArray::fromBase64( lg_user.toLocal8Bit());
    ret = DesDec( (unsigned char *)tmp.data(), tmp.size(), encUsr, &encUsrLen);
    if(ret != 0)
    {
        cout << "DesDec";
        return;
    }
    #ifdef _WIN32 //如果是windows平台
        //fromLocal8Bit(), 本地字符集转换为utf-8
        ui->lineEdit_id->setText( QString::fromLocal8Bit( (const char *)encUsr, encUsrLen ) );
    #else //其它平台
        ui->lineEdit_id->setText( (const char *)encUsr );
    #endif
    if(lg_remenber == "yes"){
        unsigned char encPwd[512] = {0};
        int encPwdLen = 0;
        //toLocal8Bit(), 转换为本地字符集，默认windows则为gbk编码，linux为utf-8编码
        QByteArray tmp = QByteArray::fromBase64( lg_pwd.toLocal8Bit());
        ret = DesDec( (unsigned char *)tmp.data(), tmp.size(), encPwd, &encPwdLen);
        if(ret != 0)
        {
            cout << "DesDec";
            return;
        }

        #ifdef _WIN32 //如果是windows平台
            //fromLocal8Bit(), 本地字符集转换为utf-8
            ui->lineEdit_pw->setText( QString::fromLocal8Bit( (const char *)encPwd, encPwdLen ) );
        #else //其它平台
            ui->lineEdit_pw->setText( (const char *)encPwd );
        #endif

        ui->checkBox_know->setChecked(true);
    }
    ui->setpage_svip->setText(sv_ip);
    ui->setpage_sport->setText(sv_port);
}

void login::on_back_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->login_page);
}

void login::on_back_1_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->login_page);

}
