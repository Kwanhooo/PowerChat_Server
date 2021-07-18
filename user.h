#ifndef USER_H
#define USER_H

#include <QString>
#include <QTcpSocket>
#include <QTcpServer>

class User
{
public:
    User(QString userName, QString password,QString email,QString phone,int status = 0,QString IP = "ip",quint16 port = 0);
    QString userName;
    QString password;
    QString email;
    QString phone;
    int status;
    QString IP;
    quint16 port;

    QStringList *friendsList;

    QString toString();
};

#endif // USER_H
