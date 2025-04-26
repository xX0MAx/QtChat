#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QListWidget>
#include <QTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostInfo>

class Server : public QTcpServer{

    Q_OBJECT

public:
    Server();
    QTcpSocket *socket;
    void Start(int port);
    void Close();
    void AdminChat(QString str);
    void DisconnectClient(QString clientId);
    void ClientList();

private:
    void SendToClient(QString str);
    QVector <QTcpSocket*> Sockets;
    QByteArray Data;
    quint16 nextBlockSize;
    int ID;

public slots:
    void incomingConnection(qintptr socketDescriptor);
    void slotReadyRead();
    void slotClientDisconnected();

signals:
    void messageReceived(const QString &message);
    void messageToChat(const QString &message);
    void messageUserList(const QString &message);
    void getUserList(const QString &message);
    void setUserList(const QStringList &userList);
};

#endif // SERVER_H
