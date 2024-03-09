#include "server.h"
#include <unordered_map>
#define PORTNUM 12345

std::unordered_map<QTcpSocket*, QString> socketToNicknameMap;
std::unordered_map<QString, QTcpSocket*> nicknameToSocketMap;

Server::Server()
{
    if(this->listen(QHostAddress::Any, PORTNUM))
    {
        qDebug() << "server started";
    }
    else
    {
        qDebug() << "Listen: error";
    }
    blockSize = 0;
}

void Server::incomingConnection(qintptr sd)
{
    socket = new QTcpSocket;
    socket->setSocketDescriptor(sd);
    connect(socket, &QTcpSocket::readyRead, this, &Server::readyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::removeSocket);

    numOfGuests++;
    QString nick = "Guest" + QString::number(numOfGuests);
    QString msg = "*** " + nick + " подключился";
    assignNicknameToSocket(nick, socket);
    Nicknames.push_back(nick);
    Sockets.push_back(socket);
    qDebug() << nick << "connected";
    SendMess("", OnlineListMessage);
    SendMess(msg, ServerMessage);
}

void Server::readyRead()
{
    socket = (QTcpSocket*)sender();
    QDataStream in(socket);
    MessageCodes msgCode;
    if(in.status() == QDataStream::Ok)
    {
        qDebug() << "waiting...";
        for(;;)
        {
            if(blockSize == 0)
            {
                qDebug() << "blocksize = 0";
                if(socket->bytesAvailable() < 2)
                {
                    qDebug() << "Message < 2";
                    break;
                }
                in >> blockSize;
                qDebug() << "blocksize = " << blockSize;
            }
            if(socket->bytesAvailable() < blockSize)
            {
                qDebug() << "its not all";
                break;
            }

            QString a, msg;
            QString oldNickname, newNickname, senderNick; //wrote here because of switch-case, this operator uses jumps
            in >> msgCode;
            in >> a;
            switch (msgCode) {
                case RegularMessage:
                    qDebug() << "RegularMessage:" << a;
                    SendMess(a);
                    break;

                //someday if i will need it
                /*case NewNicknameMessage:
                    qDebug() << "NewNicknameMessage:" << a;
                    nick = findNicknameBySocket(socket);
                    msg = "*** " + nick + " поменял никнейм на " + a;
                    removeSocketAndNickname(socket, nick);
                    assignNicknameToSocket(a, socket);
                    Nicknames.erase(std::remove(Nicknames.begin(), Nicknames.end(), nick), Nicknames.end());
                    Nicknames.push_back(a);
                    SendMess("", OnlineListMessage);
                    SendMess(msg, ServerMessage);
                    break;*/

                case NicknameMessage: case NewNicknameMessage:
                    //in "a" old nickname
                    if(msgCode == NewNicknameMessage)
                    {
                        oldNickname = findNicknameBySocket(socket);
                        in >> newNickname;
                        qDebug() << "NewNicknameMessage:" << newNickname;
                    }
                    else
                    {
                        in >> newNickname;
                        oldNickname = a;
                        qDebug() << "NicknameMessage:" << oldNickname << "->" << newNickname;
                    }
                    msg = "*** " + oldNickname + " поменял никнейм на " + newNickname;
                    removeSocketAndNickname(socket, oldNickname);
                    assignNicknameToSocket(newNickname, socket);
                    Nicknames.erase(std::remove(Nicknames.begin(), Nicknames.end(), oldNickname), Nicknames.end());
                    Nicknames.push_back(newNickname);
                    SendMess("", OnlineListMessage);
                    SendMess(msg, ServerMessage);
                    break;

                case PrivateMessage:
                    //in "a" private nickname
                    senderNick = findNicknameBySocket(socket);
                    in >> msg;
                    qDebug() << "PrivateMessage:" << senderNick << "with:" << a << "with message:" << msg;
                    SendPrivateMess(senderNick, a, msg);
                    SendPrivateMess(a, senderNick, msg);
                    break;
            }
            blockSize = 0;
            break;
        }
    }
    else
    {
        qDebug() << "DataStream error";
    }
}

void Server::SendMess(QString a, MessageCodes msgCode)
{
    Message.clear();
    QDataStream out(&Message, QIODevice::WriteOnly);
    if(msgCode == RegularMessage)
    {
        QString nick = findNicknameBySocket(socket);
        if (nick == "")
        {
            nick = "Guest";
        }
        out << quint16(0) << msgCode << nick << a;
    }
    else if(msgCode == OnlineListMessage)
    {
        out << quint16(0) << msgCode << Nicknames.size();
        for(int i = 0; i < Nicknames.size(); i++)
        {
            out << Nicknames[i];
        }
    }
    else
    {
        out << quint16(0) << msgCode << a;
    }
    out.device()->seek(0);
    out << quint16(Message.size() - sizeof(quint16));
    for(int i = 0; i < Sockets.size(); i++)
    {
        Sockets[i]->write(Message);
    }
}

void Server::removeSocket()
{
    socket = (QTcpSocket*)sender();
    Sockets.erase(std::remove(Sockets.begin(), Sockets.end(), socket), Sockets.end());
    QString nick = findNicknameBySocket(socket);
    QString msg = "*** " + nick + " отключился";
    qDebug() << nick << "disconnected";
    Nicknames.erase(std::remove(Nicknames.begin(), Nicknames.end(), nick), Nicknames.end());
    removeSocketAndNickname(socket, nick);
    socket->deleteLater();
    SendMess("", OnlineListMessage);
    SendMess(msg, ServerMessage);
}

void Server::assignNicknameToSocket(const QString& nickname, QTcpSocket* socket)
{
    socketToNicknameMap[socket] = nickname;
    nicknameToSocketMap[nickname] = socket;
}

QString Server::findNicknameBySocket(QTcpSocket* socket)
{
    auto it = socketToNicknameMap.find(socket);
    if (it != socketToNicknameMap.end())
    {
        return it->second;
    }
    return "";
}

QTcpSocket* Server::findSocketByNickname(const QString& nickname)
{
    auto it = nicknameToSocketMap.find(nickname);
    if (it != nicknameToSocketMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void Server::removeSocketAndNickname(QTcpSocket* socket, const QString& nickname)
{
    auto socketIt = socketToNicknameMap.find(socket);
    if (socketIt != socketToNicknameMap.end())
    {
        socketToNicknameMap.erase(socketIt);
    }

    auto nicknameIt = nicknameToSocketMap.find(nickname);
    if (nicknameIt != nicknameToSocketMap.end())
    {
        nicknameToSocketMap.erase(nicknameIt);
    }
}

void Server::SendPrivateMess(QString nicknameToWho, QString nicknameFromWho, QString msg)
{
    QTcpSocket *sock = findSocketByNickname(nicknameToWho);
    Message.clear();
    QDataStream out(&Message, QIODevice::WriteOnly);
    out << quint16(0) << PrivateMessage << nicknameFromWho << msg;
    out.device()->seek(0);
    out << quint16(Message.size() - sizeof(quint16));
    sock->write(Message);
}
