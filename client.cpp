#include "client.h"

Client::Client(QString IP, int port){
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &Client::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    nextBlockSize = 0;
    Name = "Anonim";
    socket->connectToHost(IP, port);
}

void Client::disconnectFromServer(){
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
}

void Client::SendToServer(QString str){
    if (socket->state() == QAbstractSocket::ConnectedState){
        Data.clear();
        QDataStream out(&Data, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_2);
        out << quint16(0) << QTime::currentTime() << str;
        out.device()->seek(0);
        out << quint16(Data.size()-sizeof(quint16));
        socket->write(Data);
    }
    else{
        emit messageToClientChat(QString("You disconnected from server"));
    }
}

void Client::Chat(QString str){
    if(Name == ""){
        Name = "Anonim";
    }
    QString message = Name + ":\n" + "    " + str;
    SendToServer(message);
}

void Client::slotReadyRead(){
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_2);

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
            emit messageToClientChat(QString("%1, %2").arg(time.toString("h:m")).arg(str));
        }
    }
}
