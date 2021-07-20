#ifndef POWERSERVER_H
#define POWERSERVER_H

#include <QMainWindow>

#include <QMessageBox>

#include <QTcpServer>
#include <QTcpSocket>
#include <QtNetwork>
#include <QtGui>

#include <QTime>
#include <QTimer>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QList>

#include "user.h"
#include "offlinebuffer.h"

namespace Ui {
class PowerServer;
}

class PowerServer : public QMainWindow
{
    Q_OBJECT

public:
    explicit PowerServer(QWidget *parent = nullptr);
    ~PowerServer();

private slots:
    void on_btn_save_clicked();
    void on_btn_addUser_clicked();
    void on_btn_deleteUser_clicked();

    void on_btn_clear_attendance_clicked();

private:
    Ui::PowerServer *ui;

    const static int MAX_USERS_AMOUNT = 20;
    int currentUserAmount;

    User *userList[MAX_USERS_AMOUNT];
    int userListSize;

    QTcpSocket *tcpSocket[MAX_USERS_AMOUNT];
    QTcpServer *tcpServer;

    OfflineBuffer *offlineBuffer;

    //打卡模块
    QStringList attendaceUsers;

    void loadUserConfig();
    void saveUserConfig();

    void initParameter();
    void initUILogic();

    void showServerConfig();
    void setupTcp();

    void dealWithMessage(QString response);
    void sendOfflineMessage(QString requestUserName,QTcpSocket *tcpSocket);//处理离线消息

    void updateServerInterface();
    void updateClientInterface(QString requestUserName);//仅指定的requestUserName接受一次用户列表的刷新
    void updateClientInterface();//所有Client都接受一次用户列表的更新
};

#endif // POWERSERVER_H
