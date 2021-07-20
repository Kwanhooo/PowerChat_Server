#include "powerserver.h"
#include "ui_powerserver.h"

/*
 * 以下代码段为构造函数和析构函数
 */
PowerServer::PowerServer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PowerServer)
{
    ui->setupUi(this);

    //初始化一些必要参数
    this->initParameter();

    //从UserListConfig.user文件中加载用户信息
    this->loadUserConfig();

    //更新服务器用户列表
    this->updateServerInterface();

    //设置起始的UI逻辑
    this->initUILogic();

    //在控制台输出服务端的网络信息（调试用）
    this->showServerConfig();
    this->setupTcp();

}

PowerServer::~PowerServer()
{
    delete ui;
}


/*
 * 以下代码段为运行时所需要的函数
 */
void PowerServer::initParameter()
{
    //初始化用户列表
    for (int i = 0;i < MAX_USERS_AMOUNT;i++)
    {
        userList[i] = nullptr;
    }
    currentUserAmount = 0;
    userListSize = 0;
    offlineBuffer = new OfflineBuffer();
    qDebug()<<"initParameter completed"<<endl;
}

void PowerServer::initUILogic()
{
    ui->lineEdit_IP->setReadOnly(true);
    ui->lineEdit_port->setReadOnly(true);
    ui->lineEdit_online->setReadOnly(true);
    ui->lineEdit_offline->setReadOnly(true);
    ui->textBrowser_log->setReadOnly(true);
    ui->textBrowser_online->setReadOnly(true);
    ui->textBrowser_offline->setReadOnly(true);
}

void PowerServer::showServerConfig()
{
    QString localHostName = QHostInfo::localHostName();
    qDebug() <<"localHostName: "<<localHostName;
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach(QHostAddress address,info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            qDebug() << address.toString();
            ui->textBrowser_log->append(address.toString());
        }
    }
}

//设置TCP连接
void PowerServer::setupTcp()
{
    for (int i = 0;i < MAX_USERS_AMOUNT;i++)
    {
        tcpSocket[i] = new QTcpSocket;
    }

    tcpServer = new QTcpServer;
    tcpServer->listen(QHostAddress::Any,10086);
    this->setWindowTitle(QString("PowerChat Server (LISTENING ON::120.78.235.195:10086)"));

    connect(tcpServer,&QTcpServer::newConnection,[=]()
    {
        //将新连接加入socket数组并更新用户数量
        int socketIndex = currentUserAmount;
        tcpSocket[socketIndex] = tcpServer->nextPendingConnection();
        currentUserAmount++;

        //读取新连接用户的IP以及端口号
        QString newIP = tcpSocket[socketIndex]->peerAddress().toString().section(":",3,3);
        quint16 newPort = tcpSocket[socketIndex]->peerPort();

        //更新日志
        QString logNewConnection = QString("[%1:%2]:成功连接").arg(newIP).arg(newPort);
        ui->textBrowser_log->append(logNewConnection);

        connect(tcpSocket[socketIndex],&QTcpSocket::connected,[=]()
        {
            ui->btn_save->setEnabled(false);
        });

        connect(tcpSocket[socketIndex],&QTcpSocket::disconnected,[=]()
        {
            QString logDisconnected = QString("[%1:%2]:断开连接").arg(newIP).arg(newPort);
            ui->textBrowser_log->append(logDisconnected);

            for(int i=0; i< userListSize; i++)
            {
                if(userList[i]->IP == newIP)
                {
                    ui->textBrowser_log->append(QString("%1离线了").arg(userList[i]->userName));
                    userList[i]->status = 0; //离线
                    break;
                }
            }
            if(socketIndex < currentUserAmount - 1)
            {
                for(int i = socketIndex; i< currentUserAmount - 1; i++)
                {
                    tcpSocket[i] = tcpSocket[i+1];
                }
            }
            currentUserAmount --;
            //有人断开连接了
            //Server端重新从userList中更新UI
            this->updateServerInterface();
            //向客户端广播最新的用户状态
            this->updateClientInterface();
        });

        connect(tcpSocket[socketIndex],&QTcpSocket::readyRead,[=]()
        {
            QByteArray bufferArea = tcpSocket[socketIndex]->readAll();
            QString response(bufferArea);
            qDebug() << endl<<"******RESPONSE******" << response<<endl;
            ui->textBrowser_log->append("<font color=red>******RESPONSE******</font>");
            ui->textBrowser_log->append(QString("<font color=red>%1</font>").arg(response));
            if(response.at(0)=='#'&&response.at(1)=='#')//这是一个指令信息
            {
                qDebug()<<"这是一个指令信息"<<endl;
                ui->textBrowser_log->append("<font color=orange>这是一个指令消息</font>");

                if(response.section("##",1,1)=="LOGIN_REQUEST")//##LOGIN_REQUEST
                {
                    tcpSocket[socketIndex]->write("##LOGIN_PERMISSION");
                }


                else if(response.section("##",1,1)=="LOGIN_CERTIFICATE")//##LOGIN_CERTIFICATE##usesrName##password
                {
                    QString userNameToValidate = response.section("##",2,2);
                    QString passwordToValidate = response.section("##",3,3);

                    bool isMatch = false;

                    for (int i = 0;i < userListSize;i++)
                    {
                        if(userList[i]->userName == userNameToValidate&&userList[i]->password == passwordToValidate)
                        {
                            isMatch = true;
                            //设定新上线的用户信息
                            userList[i]->IP = newIP;
                            userList[i]->port = newPort;
                            userList[i]->status = 1;
                            //写入日志
                            ui->textBrowser_log->append(QString("%1现已上线").arg(userNameToValidate));

                            break;
                        }
                    }

                    if(isMatch)
                        tcpSocket[socketIndex]->write(QString("##LOGIN_SUCCESS##%1").arg(userNameToValidate).toUtf8());
                    else
                        tcpSocket[socketIndex]->write(QString("##LOGIN_FAILED").toUtf8());
                }


                else if(response.section("##",1,1) == "REGISTER_CERTIFICATE")
                    //##REGISTER_CERTIFICATE##usesrName##password##email##phone
                {
                    QString userNameReg = response.section("##",2,2);
                    QString passwordReg = response.section("##",3,3);
                    QString emailReg = response.section("##",4,4);
                    QString phoneReg = response.section("##",5,5);

                    bool isSame = false;
                    for (int i = 0;i < userListSize;i++)
                    {
                        if(userList[i]->userName == userNameReg)
                        {
                            isSame = true;
                            break;
                        }
                    }
                    if(isSame)
                        tcpSocket[socketIndex]->write(QString("##REGISTER_FAILED").toUtf8());
                    else
                    {
                        userList[userListSize] = new User(userNameReg,passwordReg,emailReg,phoneReg,1,newIP,newPort);
                        userListSize++;

                        tcpSocket[socketIndex]->write(QString("##REGISTER_SUCCESS##%1").arg(userNameReg).toUtf8());
                    }
                }


                else if(response.section("##",1,1)=="REQUEST_USER_CONFIG")//从Client登录之后主动向服务端请求更新好友列表
                    //##REQUEST_USER_CONFIG##USERNAME
                {
                    QString requestUserName = response.section("##",2,2);
                    this->updateServerInterface();
                    this->updateClientInterface(requestUserName);
                    ui->btn_save->setEnabled(true);
                }


                else if(response.section("##",1,1)=="GET_OFFLINE_MESSAGE")
                {
                    QString requestUserName = response.section("##",2,2);
                    ui->textBrowser_log->append("准备调起sendOfflineMessage()");
                    this->sendOfflineMessage(requestUserName,tcpSocket[socketIndex]);
                }


                else if(response.section("##",1,1)=="STATUS_CHANGE_REQUEST")
                    //##STATUS_CHANGE_REQUEST##USERNAME##STATUS(INT)
                {
                    QString requestUserName = response.section("##",2,2);
                    int statusToChange = response.section("##",3,3).toInt();

                    for (int i = 0;i < userListSize;i++)
                    {
                        if(userList[i]->userName == requestUserName)
                        {
                            userList[i]->status = statusToChange;
                            this->updateServerInterface();//更新Server端UI
                            this->updateClientInterface();//向Client广播
                            break;
                        }
                    }
                }


                else if(response.section("##",1,1) == "ADD_REQUEST")
                    //##ADD_REQUEST##USERNAME##FRIENDNAME
                {
                    QString requester = response.section("##",2,2);
                    QString friendNameToFind = response.section("##",3,3);

                    bool isFound = false;
                    int indexOfFriend = 0;
                    for (int i = 0;i < userListSize;i++)
                    {
                        if(userList[i]->userName == friendNameToFind)
                        {
                            isFound = true;
                            indexOfFriend = i;
                            break;
                        }
                    }

                    //如果找到了friendToName的这个用户，向requester发送反馈，并将请求发送给friend
                    if(isFound)
                    {
                        tcpSocket[socketIndex]->write(QString("##IS_FRIEND_FOUND##%1##TRUE").arg(friendNameToFind).toUtf8());
                        ui->textBrowser_log->append(QString("已找到用户%1（搜索请求来自%2）").arg(friendNameToFind).arg(requester));

                        ui->textBrowser_log->append(QString("现在将来自%1的好友请求发送至%2").arg(requester).arg(friendNameToFind));
                        //将要发送至好友的请求命令
                        //##NEW_FRIEND_REQUEST##REQUESTER
                        QString requestCommand = "##NEW_FRIEND_REQUEST##";
                        requestCommand.append(requester);

                        //如果对方在线，则直接找到他的socket发送请求
                        if(userList[indexOfFriend]->status != 0)
                        {
                            for (int i = 0;i < currentUserAmount;i++)
                            {
                                if(tcpSocket[i]->peerAddress().toString().section(":",3,3) == userList[indexOfFriend]->IP &&
                                        tcpSocket[i]->peerPort() == userList[indexOfFriend]->port)
                                {
                                    tcpSocket[i]->write(requestCommand.toUtf8());
                                    ui->textBrowser_log->append(QString("消息已发送至%1:%2").arg(tcpSocket[i]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[i]->peerPort()));
                                }
                            }
                        }
                        //如果对方不在线，将请求放入offlineBuffer中，待其上线后发送
                        else
                        {
                            //需要更换标志符号，重制命令，因为##会与OFFLINE_MESSAGE命令冲突
                            QString offlineRequestCommand = "@@NEW_FRIEND_REQUEST@@";
                            offlineRequestCommand.append(requester);
                            //命令的最终形式
                            //@@NEW_FRIEND_REQUEST@@REQUESTER
                            offlineBuffer->addMsg(requester,friendNameToFind,offlineRequestCommand);
                        }

                    }
                    //如果没有找到这个用户，反馈给requester，告知其查无此人
                    else
                    {
                        tcpSocket[socketIndex]->write(QString("##IS_FRIEND_FOUND##%1##FALSE").arg(friendNameToFind).toUtf8());
                        ui->textBrowser_log->append(QString("未找到用户%1（搜索请求来自%2）").arg(friendNameToFind).arg(requester));
                    }
                }


                else if(response.section("##",1,1) == "FRIEND_REQUEST_RETURN")
                    //##FRIEND_REQUEST_RETURN##REQUESTER##FRIENDNAME##STATUS
                {
                    QString requester = response.section("##",2,2);
                    QString friendName = response.section("##",3,3);
                    QString requestStatus = response.section("##",4,4);

                    int indexOfRequester = 0;
                    if(requestStatus == "ACCEPTED")
                    {
                        //找到请求者和准好友的userList位置，更新两者userList中的好友列表
                        for (int i = 0;i < userListSize;i++)
                        {
                            if(userList[i]->userName == requester)
                            {
                                userList[i]->friendsList->append(friendName);
                                indexOfRequester = i;
                                continue;
                            }
                            if(userList[i]->userName == friendName)
                            {
                                userList[i]->friendsList->append(requester);
                            }
                        }

                        //刷新两者的客户端UI
                        updateClientInterface(requester);
                        updateClientInterface(friendName);
                    }

                    //现在通知请求者:对方是否同意了请求
                    //如果请求者在线，则直接找到他的socket发送请求
                    if(userList[indexOfRequester]->status != 0)
                    {
                        QString requestReturnCommand = "##FRIEND_REQUEST_STATUS##";
                        requestReturnCommand.append(friendName);
                        requestReturnCommand.append("##");
                        requestReturnCommand.append(requestStatus);
                        //最终命令形式
                        //##FRIEND_REQUEST_STATUS##FRIENDNAME##STATUS
                        for (int i = 0;i < currentUserAmount;i++)
                        {
                            if(tcpSocket[i]->peerAddress().toString().section(":",3,3) == userList[indexOfRequester]->IP &&
                                    tcpSocket[i]->peerPort() == userList[indexOfRequester]->port)
                            {
                                tcpSocket[i]->write(requestReturnCommand.toUtf8());
                                ui->textBrowser_log->append(QString("消息已发送至%1:%2").arg(tcpSocket[i]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[i]->peerPort()));
                            }
                        }
                    }
                    //如果对方不在线，将请求放入offlineBuffer中，待其上线后发送
                    else
                    {
                        //需要更换标志符号，重制命令，因为##会与OFFLINE_MESSAGE命令冲突
                        QString offlineRequestCommand = "@@FRIEND_REQUEST_STATUS@@";
                        offlineRequestCommand.append(friendName);
                        offlineRequestCommand.append(requestStatus);
                        //最终命令形式
                        //@@FRIEND_REQUEST_STATUS@@FRIENDNAME@@STATUS
                        offlineBuffer->addMsg(requester,friendName,offlineRequestCommand);
                    }
                }
                //不在设计内的，以##开头的未知指令
                else
                {
                    ui->textBrowser_log->append("收到了一条以##开头的位置命令，没有进行任何处理");
                    qDebug()<<"收到了一条以##开头的位置命令，没有进行任何处理"<<endl;
                }
            }
            //这是一个中转消息
            else
            {
                dealWithMessage(response);
            }
        });
    });

}

void PowerServer::dealWithMessage(QString response)
{
    qDebug()<<"这是一个中转消息"<<endl;
    ui->textBrowser_log->append("<font color=orange>这是一个中转消息</font>");
    QString senderName = response.section("##",0,0);
    QString msg = response.section("##",1,1);
    QString recipientName = response.section("##",2,2);

    ui->textBrowser_log->append(QString("收到中转消息，原文：%1").arg(response));
    ui->textBrowser_log->append(QString("发送者%1，接收者%2").arg(senderName).arg(recipientName));

    for (int i = 0;i < userListSize;i++)//从总用户列表中找到接收者
    {
        if(userList[i]->userName==recipientName)
        {
            ui->textBrowser_log->append(QString("已找到接收者：%1").arg(userList[i]->userName));

            if(userList[i]->status>=1)//如果接收者在线，隐身对于Server端也算是在线，按照在线的方式发送
            {
                ui->textBrowser_log->append(QString("接收者在线"));
                for (int j = 0;j < currentUserAmount;j++)//搜索socket列表，找到接收者的socket
                {
                    ui->textBrowser_log->append(QString("peerAddress = %1       peerPort = %2").arg(tcpSocket[j]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[j]->peerPort()));

                    if(tcpSocket[j]->peerAddress().toString().section(":",3,3)==userList[i]->IP
                            &&tcpSocket[j]->peerPort()==userList[i]->port)
                    {

                        ui->textBrowser_log->append(QString("消息已发送至%1:%2").arg(tcpSocket[j]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[j]->peerPort()));
                        tcpSocket[j]->write(response.toUtf8());
                        break;
                    }
                }
            }
            else//如果接收者离线，将消息放进队列中，待其上线后请求获取
            {
                ui->textBrowser_log->append("接收者离线，已经存入消息缓存区");

                //COMMAND FORMAT::
                //##OFFLINE_MESSAGE##amount##senderName&&msg&&recipientName(MSG1)##(MSG2)##....

                offlineBuffer->addMsg(senderName,recipientName,msg);
                ui->textBrowser_log->append("缓存头中的消息：");
                ui->textBrowser_log->append(offlineBuffer->head->msg);
            }
            break;
        }
    }
}

void PowerServer::sendOfflineMessage(QString requestUserName,QTcpSocket *tcpSocket)
{
    //向请求的Client发送上线前的消息
    ui->textBrowser_log->append(QString("搜索属于%1的离线消息...").arg(requestUserName));

    //从缓存区中搜索所有属于requestUserName的消息
    int amount = 0;
    BufferNode *a = offlineBuffer->head;
    QString totalMessage = "";

    while(a!=nullptr)
    {
        qDebug()<<QString("收:%1  发:%2  消息:%3").arg(a->sender).arg(a->recipient).arg(a->msg)<<endl;
        if(a-> recipient == requestUserName)
        {
            totalMessage.append(QString("%1&&%2&&%3##").arg(a->sender).arg(a->msg).arg(a->recipient));
            amount++;
            a->isSent = true;
            qDebug()<<QString("找到了相关离线消息，现有%1条").arg(amount)<<endl;
        }
        a = a->next;
    }

    if(amount != 0)
    {
        ui->textBrowser_log->append(QString("共有%1条离线消息，现在发送至Client").arg(amount));
        QString command = QString("##OFFLINE_MESSAGE##%1##%2").arg(amount).arg(totalMessage);
        tcpSocket->write(command.toUtf8());
        //##OFFLINE_MESSAGE##amount##senderName&&msg&&recipientName(MSG1)##(MSG2)##....
    }
    else
    {
        QString command = QString("##OFFLINE_MESSAGE##0");
        tcpSocket->write(command.toUtf8());
    }

    ui->textBrowser_log->append("离线消息发送完成");

    //清理已经发送过的消息
    //2021.7.18  --- 清理缓存区的函数出了问题，发送两条及以上的离线消息可能会炸开，要么就是拉取离线消息时炸开
    //    offlineBuffer->deleteSentMsg();
}

void PowerServer::updateServerInterface()
{
    qDebug() << "现在更新用户数据：" << userListSize;
    //先重置Server端的UI显示
    ui->lineEdit_offline->clear();
    ui->lineEdit_online->clear();
    ui->textBrowser_offline->clear();
    ui->textBrowser_online->clear();
    int onlineAmount = 0, offlineAmount=0;


    QString updateCommand = QString("##UPDATE_USER_CONFIG##%1##").arg(userListSize);//准备给Client发送的更新用户列表的指令

    //之后遍历用户列表，更新UI中的用户列表
    for(int i = 0; i< userListSize; i++)
    {
        updateCommand = updateCommand.append(userList[i]->toString());
        if(userList[i]->status >= 1)
        {
            if(userList[i]->status == 1)
                ui->textBrowser_online->append(userList[i]->userName);
            if(userList[i]->status == 2)
                ui->textBrowser_online->append(userList[i]->userName);
            if(userList[i]->status == 3)
                ui->textBrowser_online->append(userList[i]->userName);
            if(userList[i]->status == 4)
                ui->textBrowser_online->append(userList[i]->userName);
            if(userList[i]->status == 5)
                ui->textBrowser_online->append(userList[i]->userName);
            if(userList[i]->status == 6)
                ui->textBrowser_online->append(userList[i]->userName);
            onlineAmount++;
        }
        else
        {
            ui->textBrowser_offline->append(userList[i]->userName);
            offlineAmount++;
        }
    }
    ui->lineEdit_online->setText(QString::number(onlineAmount, 10));//用法：QString::number(string,int(base))
    ui->lineEdit_offline->setText(QString::number(offlineAmount, 10));
}

void PowerServer::updateClientInterface(QString requestUserName)
{
    ui->textBrowser_log->append(QString("向%1更新好友列表...").arg(requestUserName));
    int userIndex;
    bool isFound = false;
    //找到请求者在userList中的索引值
    for (userIndex = 0;userIndex < userListSize;userIndex++)
    {
        if(userList[userIndex]->userName == requestUserName)
        {
            isFound = true;
            break;
        }
    }
    if(!isFound)//如果没找到则说明出现了问题
    {
        QMessageBox::warning(this,"严重错误",QString("对于%1的用户列表更新请求无法应答：无法找到请求者的信息！").arg(requestUserName),QMessageBox::Yes);
        return;
    }
    else//获取到了请求者的索引值
    {
        //获得请求者的好友用户名
        QStringList friendList = *(userList[userIndex]->friendsList);

        //初始化准备发送给Client的指令
        //##UPDATE_USER_CONFIG##USERAMOUNT##INFO STREAM
        QString updateCommand = "##UPDATE_USER_CONFIG##";

        //指示好友数量
        int friendAmount = friendList.count();

        //以空格隔开每一个信息，以便生成一个字符串流
        QString infoStream;

        //遍历指定用户的好友列表，合成用户列表串
        for (QList<QString>::iterator it = friendList.begin();it != friendList.end();it++)
        {
            QString userNameToFind = *it;

            //从userList中找到userNameToFind的所有信息，合并到infoStream里
            for (int i = 0;i < userListSize;i++)
            {
                if(userNameToFind == userList[i]->userName)
                {
                    infoStream.append(userList[i]->toString());
                    break;
                }
            }
        }
        updateCommand.append(QString("%1").arg(friendAmount));
        updateCommand.append("##");
        updateCommand.append(infoStream);
        //最终命令形式：
        //##UPDATE_USER_CONFIG##USERAMOUNT##INFO STREAM

        ui->textBrowser_log->append(QString("<font color=blue>来自%1的更新用户列表请求，发送命令如下：</font>").arg(requestUserName));
        ui->textBrowser_log->append(QString("<font color=blue>%1</font>").arg(updateCommand));

        //找到请求者的socket以发送指令
        for (int i = 0;i < currentUserAmount;i++)
        {
            if(tcpSocket[i]->peerAddress().toString().section(":",3,3) == userList[userIndex]->IP&&
                    tcpSocket[i]->peerPort() == userList[userIndex]->port)
            {
                if(tcpSocket[i]->isOpen())
                {
                    tcpSocket[i]->write(updateCommand.toUtf8());
                    ui->textBrowser_log->append(QString("<font color=green>命令已发送至%1:%2</font>").arg(tcpSocket[i]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[i]->peerPort()));
                }
                else
                {
                    ui->textBrowser_log->append(QString("<font color=red>无法发送命令，因为该TcpSocket已关闭"));
                }
                break;
            }
        }
    }
}

void PowerServer::updateClientInterface()
{
    ui->textBrowser_log->append("向所有Client更新好友列表...");
    for (int i = 0;i < currentUserAmount;i++)
    {
        //从userList中找到现在tcpSocket对应的用户，获取其好友列表
        QStringList friendList;
        for (int j = 0;j < userListSize;j++)
        {
            if(tcpSocket[i]->peerAddress().toString().section(":",3,3) == userList[j]->IP &&
                    tcpSocket[i]->peerPort() == userList[j]->port)
            {
                friendList = *(userList[j]->friendsList);
                break;
            }
        }

        //遍历当前用户的好友列表friendList，合成用户列表串infoStream
        QString updateCommand = "##UPDATE_USER_CONFIG##";
        QString infoStream;
        int friendAmount = friendList.count();
        for (QList<QString>::iterator it = friendList.begin();it != friendList.end();it++)
        {
            QString userNameToFind = *it;

            //从userList中找到userNameToFind的所有信息，合并到infoStream里
            for (int k = 0;k < userListSize;k++)
            {
                if(userNameToFind == userList[k]->userName)
                {
                    infoStream.append(userList[k]->toString());
                    break;
                }
            }
        }
        //合成指令
        updateCommand.append(QString("%1").arg(friendAmount));
        updateCommand.append("##");
        updateCommand.append(infoStream);
        if(tcpSocket[i]->isOpen())
        {
            tcpSocket[i]->write(updateCommand.toUtf8());
            ui->textBrowser_log->append(QString("<font color=green>命令已发送至%1:%2</font>").arg(tcpSocket[i]->peerAddress().toString().section(":",3,3)).arg(tcpSocket[i]->peerPort()));
        }
        else
        {
            ui->textBrowser_log->append(QString("<font color=red>无法发送命令，因为该TcpSocket已关闭"));
        }
    }
}

void PowerServer::loadUserConfig()
{
    QFile config("UserListConfig.user");
    qDebug() << "读取用户信息...";
    if(config.open(QIODevice::ReadOnly | QIODevice :: Text))
    {
        QByteArray a = QByteArray::fromBase64(config.readAll());
        QString b = QString(a);
        QTextStream read(&b);

        QString userName = "";
        QString password = "";
        QString email = "";
        QString phone = "";
        userListSize = 0;

        while(!read.atEnd())
        {
            read >> userName;
            if(userName != "")
            {
                qDebug() << userName;
                read >> password;
                read >> email;
                read >> phone;
                userList[userListSize] = new User(userName, password, email, phone);
                userListSize++;
                qDebug()<<"userListSize = "<< userListSize <<endl;
            }
            else
            {
                break;
            }
        }
        config.close();

    }
    else
    {
        QMessageBox::warning(this, "读取错误", "用户列表配置文件打开失败！", QMessageBox::Yes);
        close();
    }
}


/*
 * 以下代码段为槽函数
*/
void PowerServer::on_btn_save_clicked()//将用户列表保存至外部文档
{
    //文件保存
    QFile config("UserListConfig.user");
    if(config.open(QIODevice::WriteOnly)) //以文本文式写入
    {
        QByteArray output;
        for(int i = 0; i< userListSize; i++)
        {
            output.append(userList[i]->userName.toUtf8());
            output.append(QString(" ").toUtf8());
            output.append(userList[i]->password.toUtf8());
            output.append(QString(" ").toUtf8());
            output.append(userList[i]->email.toUtf8());
            output.append(QString(" ").toUtf8());
            output.append(userList[i]->phone.toUtf8());
            output.append(QString(" ").toUtf8());
        }
        config.write(output.toBase64());
        config.close();
    }
    else
    {
        QMessageBox::warning(this, "输出错误", "用户列表配置文件打开失败！", QMessageBox::Yes);
        close();
    }
}

void PowerServer::on_btn_addUser_clicked()
{
    if(ui->lineEdit_userName->text().trimmed().isEmpty()||ui->lineEdit_password->text().trimmed().isEmpty()||
            ui->lineEdit_email->text().trimmed().isEmpty()||ui->lineEdit_phone->text().trimmed().isEmpty())
    {
        QMessageBox::information(this,"添加用户错误","用户信息输入不完整，请检查后再次添加");
        return;
    }
    ui->textBrowser_offline->append(ui->lineEdit_userName->text());
    userList[userListSize] = new User(ui->lineEdit_userName->text(), ui->lineEdit_password->text(), ui->lineEdit_email->text(), ui->lineEdit_phone->text());
    userListSize++;

    ui->lineEdit_userName->clear();
    ui->lineEdit_password->clear();
    ui->lineEdit_email->clear();
    ui->lineEdit_phone->clear();

    updateServerInterface();//更新服务器端的UI
}

void PowerServer::on_btn_deleteUser_clicked()
{
    QString deleteUserName = ui->lineEdit_deleteName->text();
    int deleteIndex = -1;
    bool isFound = false;
    for (int i = 0;i < userListSize;i++)
    {
        if(userList[i]->userName==deleteUserName)
        {
            deleteIndex = i;
            isFound = true;
            break;
        }
    }
    if(!isFound)
    {
        QMessageBox::information(this,"错误",QString("没有您所指定的用户%1,请检查！").arg(deleteUserName));
        return;
    }
    else
    {
        //将deleteIndex之后的向前移动
        //0 1 2 3 5 6                   7
        for (int i = deleteIndex;i < userListSize;i++)
        {
            userList[i]=userList[i+1];
        }
        userList[userListSize] = nullptr;
        userListSize--;
        updateServerInterface();//更新Server端的UI
        ui->lineEdit_deleteName->clear();
    }
}
