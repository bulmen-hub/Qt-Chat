#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QString>
#include <QVector>

class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server();
    QTcpSocket *socket;

private:
    QVector <QTcpSocket*> Sockets;
    QVector <QString> Nicknames;
    QByteArray Message;
    int numOfGuests = 0;
    void assignNicknameToSocket(const QString& nickname, QTcpSocket* socket);
    QString findNicknameBySocket(QTcpSocket* socket);
    QTcpSocket* findSocketByNickname(const QString& nickname);
    void removeSocketAndNickname(QTcpSocket* socket, const QString& nickname);
    enum MessageCodes
    {
        RegularMessage, NicknameMessage, NewNicknameMessage, OnlineListMessage, ServerMessage, PrivateMessage
    };
    void SendMess(QString a, MessageCodes msgCode = RegularMessage);
    void SendPrivateMess(QString nicknameToWho, QString nicknameFromWho, QString msg);
    quint16 blockSize;

public slots:
    void incomingConnection(qintptr sd);
    void readyRead();
    void removeSocket();
};

#endif // SERVER_H
