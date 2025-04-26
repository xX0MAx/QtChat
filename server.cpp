#include "server.h"

Server::Server(){

    nextBlockSize = 0;
    ID = 0;
}

void Server::Start(int port){
    QTime time = QTime::currentTime();
    if(this->listen(QHostAddress::Any, port)){
        QString localIp;
        QList<QHostAddress> addresses = QHostInfo::fromName(QHostInfo::localHostName()).addresses();
        for (const QHostAddress &address : addresses) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                localIp = address.toString();
            }
        }
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
            QString externalIp;
            if (reply->error() == QNetworkReply::NoError) {
                externalIp = reply->readAll();
            }
            else {
                externalIp = "Failed to get external IP.";
            }
            emit messageReceived(QString("{\n    Server start\nPort:%1\nLocal ip:%2\nExternal ip:%3\nTime:%4\n}").arg(port).arg(localIp).arg(externalIp).arg(time.toString()));
            reply->deleteLater();
        });
        manager->get(QNetworkRequest(QUrl("http://api.ipify.org")));
        }
        else {
        emit messageReceived(QString("{\n    Error start, time:%1\n}").arg(time.toString()));
    }
}

void Server::Close(){
    QTime time = QTime::currentTime();
    if(this->isListening()){
        this->close();
        emit messageReceived(QString("{\n    Server stop, time:%1\n}").arg(time.toString()));
    }
    else{
        emit messageReceived(QString("Error stop, time:%1").arg(time.toString()));
    }
}

void Server::incomingConnection(qintptr socketDescriptor){
    QTime time = QTime::currentTime();

    socket = new QTcpSocket;
    ID++;
    socket->setProperty("ID", ID);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::slotClientDisconnected);

    Sockets.push_back(socket);
    emit messageReceived(QString("{\n    Connected ID: %1,\n    socketDescriptor: %2\nTime:%3\n}").arg(ID).arg(socketDescriptor).arg(time.toString()));
    emit messageUserList(QString::number(ID));
}

void Server::ClientList(){
    QStringList userList;
    for (QTcpSocket* socket : Sockets){
        userList << socket->property("ID").toString();
    }
    emit setUserList(userList);
}

void Server::DisconnectClient(QString clientId) {
    for (QTcpSocket* socket : Sockets) {
        if (socket->property("ID").toString() == clientId) {
            socket->disconnectFromHost();
            emit messageReceived(QString("{\n    Disconnected client ID: %1\n}").arg(clientId));
            break;
        }
    }
    ClientList();
}

void Server::slotClientDisconnected(){

    socket = (QTcpSocket*)sender();
    int DeletedID = socket->property("ID").toInt();
    Sockets.erase(std::remove(Sockets.begin(), Sockets.end(), socket), Sockets.end());
    socket->deleteLater();
    emit messageReceived(QString("{\n    Disconnected, ID: %1\n}").arg(DeletedID));
    ClientList();
}

void Server::slotReadyRead(){

    socket = (QTcpSocket*)sender();
    int UserID = socket->property("ID").toInt();
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_2);

    if(in.status()==QDataStream::Ok){
        if(in.status()==QDataStream::Ok){
            for(;;){
                if(nextBlockSize==0){
                    if(socket->bytesAvailable()<2){
                        break;
                    }
                    in >> nextBlockSize;
                }
                if(socket->bytesAvailable()<nextBlockSize){
                    break;
                }

                QString str;
                QTime time;
                in >> time >> str;
                nextBlockSize = 0;

                emit messageReceived(QString("{\n    Message from ID: %1\n    %2\n}").arg(UserID).arg(str));
                emit messageToChat(QString("%1, %2").arg(time.toString("h:m")).arg(str));

                SendToClient(str);
                break;
            }
        }
    }
}

void Server::SendToClient(QString str){

    Data.clear();
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);
    out << quint16(0) << QTime::currentTime() << str;
    out.device()->seek(0);
    out << quint16(Data.size()-sizeof(quint16));

    for(int i =0; i < Sockets.size(); i++){
        Sockets[i]->write(Data);
    }
}

void Server::AdminChat(QString str){
    QString adminMessage = QString("Admin") + ":\n" + "    " + str;
    QTime time = QTime::currentTime();
    SendToClient(adminMessage);
    emit messageReceived(QString("{\n    Message from Admin\n    %1\n}").arg(str));
    emit messageToChat(QString("%1, %2").arg(time.toString("h:m")).arg(adminMessage));
}
