#ifndef SINGLETON_H
#define SINGLETON_H
#include <QString>
#include "common.h"


#endif // SINGLETON_H
//单例类用来保存登录用户信息
class Singleton{
public:
    static Singleton *getInstance();
    void setLoginInfo( QString tmpUser, QString tmpIp, QString tmpPort,  QString token="");//设置登陆信息
    QString getUser() const;   //获取登陆用户
    QString getIp() const;     //获取服务器ip
    QString getPort() const;   //获取服务器端口
    QString getToken() const;  //获取登陆token
private:
    Singleton();
    ~Singleton();
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);
    static Singleton *instance;
    QString user;   //当前登陆用户
    QString token;  //登陆token
    QString ip;     //web服务器ip
    QString port;   //web服务器端口
};
