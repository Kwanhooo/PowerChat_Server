#ifndef POWERSERVER_H
#define POWERSERVER_H

#include <QMainWindow>

#include <QMessageBox>

#include <QTcpServer>
#include <QTcpSocket>
#include <QtNetwork>

#include <QTime>
#include <QTimer>

#include <QFile>

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

private:
    Ui::PowerServer *ui;

    const static int MAX_USERS_AMOUNT = 20;
    int currentUserAmount;

    User *userList[MAX_USERS_AMOUNT];
    int userListSize;

    QTcpSocket *tcpSocket[MAX_USERS_AMOUNT];
    QTcpServer *tcpServer;

    OfflineBuffer *offlineBuffer;

    void loadUserConfig();
    void saveUserConfig();

    void initParameter();
    void initUILogic();

    void showServerConfig();
    void setupTcp();

    void sendOfflineMessage(QString requestUserName,QTcpSocket *tcpSocket);//处理离线消息

    bool isFirstUpdate;
    void updateUserList();
};

#endif // POWERSERVER_H
