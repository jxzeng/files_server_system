#include "Singleton.h"

Singleton* Singleton::instance = new Singleton();
Singleton::Singleton(){}
Singleton::~Singleton(){}
Singleton::Singleton(const Singleton& ){}
Singleton& Singleton::operator=(const Singleton&)
{
    return *this;
}
Singleton *Singleton::getInstance()
{
    return instance;
}
void Singleton::setLoginInfo( QString tmpUser, QString tmpIp, QString tmpPort, QString token)
{
    user = tmpUser;
    ip = tmpIp;
    port = tmpPort;
    this->token = token;
}

//获取登陆用户
QString Singleton::getUser() const
{
    return user;
}

//获取服务器ip
QString Singleton::getIp() const
{
    return ip;
}

//获取服务器端口
QString Singleton::getPort() const
{
    return port;
}

//获取登陆token
QString Singleton::getToken() const
{
    return token;
}
