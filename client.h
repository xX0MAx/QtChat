#ifndef CLIENT_H
#define CLIENT_H

#include <QTcpSocket>
#include <QTime>

class Client : public QObject{

    Q_OBJECT

public:
    Client(QString IP, int port);
    void disconnectFromServer();
    void Chat(QString str);
    QString Name;

private:
    void SendToServer(QString str);
    QTcpSocket *socket;
    QByteArray Data;
    quint16 nextBlockSize;

public slots:
    void slotReadyRead();

signals:
    void messageToClientChat(const QString &message);
};

#endif // CLIENT_H
