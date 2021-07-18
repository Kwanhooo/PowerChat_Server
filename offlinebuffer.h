#ifndef OFFLINEBUFFER_H
#define OFFLINEBUFFER_H

#include <QString>
#include <QDebug>

class BufferNode
{
public:
    QString sender;
    QString recipient;
    QString msg;
    bool isSended;
    BufferNode* next;

    BufferNode(QString sender,QString recipient,QString msg,bool isSended = false)
    {
        this->sender = sender;
        this->recipient = recipient;
        this->msg = msg;
        next = nullptr;
    }
};

class OfflineBuffer
{
public:
    BufferNode* head;
    OfflineBuffer();

    void addMsg(QString sender,QString recipient,QString msg);//添加消息到缓存区中
    QString getAllMsg(QString recipient,int* amount);
//    void deleteSendedMsg();
};

#endif // OFFLINEBUFFER_H
