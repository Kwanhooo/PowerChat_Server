#include "user.h"


User::User(QString userName, QString password, QString email, QString phone, int status, QString IP, quint16 port)
{
    this->userName = userName;
    this->password = password;
    this->email = email;
    this->phone = phone;
    this->status = status;
    this->IP = IP;
    this->port = port;

    //初始化好友列表
    this->friendsList = new QStringList;
}

QString User::toString()
{
    return QString("%1 %2 %3 %4 ").arg(userName).arg(status).arg(IP).arg(port);
}
