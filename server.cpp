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
            emit messageReceived(QString("%4: Server start,port:%1,local ip:%2,external ip:%3").arg(port).arg(localIp).arg(externalIp).arg(time.toString()));
            reply->deleteLater();
        });
        manager->get(QNetworkRequest(QUrl("http://api.ipify.org")));
        }
        else {
        emit messageReceived(QString("%1: Error start").arg(time.toString()));
    }
}

void Server::Close(){
    QTime time = QTime::currentTime();
    if(this->isListening()){
        this->close();
        emit messageReceived(QString("%1: Server stop").arg(time.toString()));
    }
    else{
        emit messageReceived(QString("%1: Error stop").arg(time.toString()));
    }
}

void Server::incomingConnection(qintptr socketDescriptor){
    QTime time = QTime::currentTime();

    socket = new QTcpSocket;
    ID++;
    socket->setProperty("ID", ID);
    QHostAddress clientAddress = socket->peerAddress();
    QString ipAddress = clientAddress.toString();
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::slotClientDisconnected);

    Sockets.push_back(socket);
    emit messageReceived(QString("%3: Connected ID: %1,IP: %2").arg(ID).arg(ipAddress).arg(time.toString()));
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
    QTime time = QTime::currentTime();
    for (QTcpSocket* socket : Sockets) {
        if (socket->property("ID").toString() == clientId) {
            socket->disconnectFromHost();
            emit messageReceived(QString("%2: Disconnected by Admin, ID: %1").arg(clientId).arg(time.toString()));
            break;
        }
    }
    ClientList();
}

void Server::slotClientDisconnected(){
    QTime time = QTime::currentTime();
    socket = (QTcpSocket*)sender();
    int DeletedID = socket->property("ID").toInt();
    Sockets.erase(std::remove(Sockets.begin(), Sockets.end(), socket), Sockets.end());
    socket->deleteLater();
    emit messageReceived(QString("%2: Disconnected client ID: %1").arg(DeletedID).arg(time.toString()));
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

                emit messageReceived(QString("%3: Message from ID: %1:'%2'").arg(UserID).arg(str).arg(time.toString()));
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
    emit messageReceived(QString("%2: Message from Admin:'%1'").arg(str).arg(time.toString()));
    emit messageToChat(QString("%1, %2").arg(time.toString("h:m")).arg(adminMessage));
}
