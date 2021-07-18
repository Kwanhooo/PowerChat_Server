#include "offlinebuffer.h"

OfflineBuffer::OfflineBuffer()
{
    this->head = nullptr;
}

void OfflineBuffer::addMsg(QString sender,QString recipient, QString msg)
{
    if(this->head==nullptr)
    {
        this->head = new BufferNode(sender,recipient,msg);
        this->head->next = nullptr;
    }
    else
    {
        BufferNode* p = this->head;
        while (p->next != nullptr)
        {
            p = p->next;
        }
        p->next = new BufferNode(sender,recipient,msg);
        p->next->next = nullptr;
    }
}

QString OfflineBuffer::getAllMsg(QString requestUserName,int* amount)
{
    BufferNode *a = this->head;
    QString totalMessage = "";
    qDebug()<<"搜索属于"<<requestUserName<<"的离线消息..."<<endl;
    while(a!=nullptr)
    {
        qDebug()<<QString("收:%1  发:%2  消息:%3").arg(a->sender).arg(a->recipient).arg(a->msg)<<endl;
        if(a-> recipient == requestUserName)
        {
            totalMessage.append(QString("%1&&%2&&%3##").arg(a->sender).arg(a->msg).arg(a->recipient));
            amount++;
            a->isSended = true;
            qDebug()<<QString("找到了相关离线消息，现有%1条").arg(*amount)<<endl;
        }
        a = a->next;
    }
    return totalMessage;
}

//void OfflineBuffer::deleteSendedMsg()
//{
//    if(this->head==nullptr)
//        return;

//    while(true)
//    {
//        if(this->head->isSended)
//        {
//            BufferNode* temp = this->head;
//            this->head = this->head->next;
//            delete temp;
//        }
//        if(this->head==nullptr||this->head->isSended==false)
//            break;
//    }

//    BufferNode* pre = this->head;
//    BufferNode* cur = this->head->next;

//    while(cur!=nullptr)
//    {
//        if(cur->isSended)
//        {
//            pre->next = cur->next;
//            delete cur;
//            cur = pre->next;
//        }
//        else
//        {
//            pre = pre->next;
//            cur = cur->next;
//        }
//    }
//}

//void OfflineBuffer::deleteSendedMsg()
//{
//    BufferNode* temp = this->head;
//    while(temp != nullptr && temp->isSended) //找到第一个未被发送的信息
//    {
//        BufferNode* pre = temp;
//        temp = temp->next;
//        delete pre; //销毁空间
//        pre = nullptr;
//    }
//    if(temp == nullptr)
//    {
//        this->head = nullptr; //头结点为空
//    }
//    else
//    {
//        this->head = temp; //头结点重新定位
//        BufferNode* p = this->head; //定位未发送节点
//        BufferNode* q = this->head->next; //定位未发送节点的下一位
//        while(q != nullptr)
//        {
//            if(q->isSended) //发送过了
//            {
//                while(q!= nullptr && q->isSended) //寻找下一个未发送的节点
//                {
//                    BufferNode* later = q;
//                    q = q->next;
//                    delete later; //销毁空间
//                    later = nullptr;
//                }
//                if(q != nullptr)
//                {
//                    p = q; //q此时未发送
//                    q = p->next;
//                }
//            }
//            else
//            {
//                p = q;
//                q = p->next;
//            }
//        }
//    }
//}
