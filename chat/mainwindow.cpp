#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <unordered_map>
#include <QTabBar>
#include "privatechatform.h"
#define PORTNUM 12345

std::unordered_map<QString, int> nicknameToIndexTabMap;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    blockSize = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    socket->connectToHost("127.0.0.1", PORTNUM);
    ui->textBrowser->append("*** Ожидание подключения");
    while (socket->state() != QAbstractSocket::ConnectedState) {
        if (!socket->waitForConnected(5000)) {
            ui->textBrowser->append("*** Не удалось подключиться");
            return;
        }
    }
    ui->connectButton->hide();
    ui->textBrowser->clear();
}

void MainWindow::SendMess(QString a, MessageCodes msgCode)
{
    Message.clear();
    QDataStream out(&Message, QIODevice::WriteOnly);
    switch (msgCode)
    {
        case NicknameMessage: case NewNicknameMessage:
            out << quint16(0) << msgCode << nickname << a;
            break;

        case PrivateMessage:
            out << quint16(0) << msgCode << privateNickname << a;
            break;

        default:
            out << quint16(0) << msgCode << a;
            break;
    }
    out.device()->seek(0);
    out << quint16(Message.size() - sizeof(quint16));
    socket->write(Message);
    ui->inputText->clear();
}

void MainWindow::readyRead()
{
    QDataStream in(socket);
    MessageCodes msgCode;
    blockSize = 0;
    if(in.status() == QDataStream::Ok)
    {
       for(;;)
       {
           if(blockSize == 0)
           {
               if(socket->bytesAvailable() < 2)
               {
                   break;
               }
               in >> blockSize;
           }
           if(socket->bytesAvailable() < blockSize)
           {
               break;
           }

           QString a;
           QString nick, oldNick, newNick;
           QVector <QString> oldNicknames;
           PrivateChatForm* privateTab = nullptr;
           int indexTab;
           in >> msgCode;
           switch (msgCode) {
               case RegularMessage:
                   in >> nick >> a;
                   nick += ": " + a;
                   ui->textBrowser->append(nick);
                   break;

               case OnlineListMessage:
                   ui->onlineBrowser->clear();
                   oldNicknames = nicknames;
                   nicknames.clear();
                   int listSize;
                   in >> listSize;
                   for(int i = 0; i < listSize; i++)
                   {
                       in >> nick;
                       nicknames.push_back(nick);
                       ui->onlineBrowser->append(nick);
                   }
                   if(oldNicknames.size() == nicknames.size())
                   {
                       for(int i = 0; i < oldNicknames.size(); i++)
                       {
                           if(!nicknames.contains(oldNicknames[i]))
                           {
                               for(int j = 0; j < nicknames.size(); j++)
                               {
                                   if(!oldNicknames.contains(nicknames[j]))
                                   {
                                       qDebug() << oldNicknames[i] << nicknames[j];
                                       oldNick = oldNicknames[i];
                                       newNick = nicknames[j];
                                       indexTab = findIndexTabByNickname(oldNick);
                                       if(indexTab != -1)
                                       {
                                           privateTab = qobject_cast<PrivateChatForm*>(ui->tabWidget->widget(indexTab));
                                           privateTab->nickname = newNick;
                                           removeIndexTabAndNickname(oldNick);
                                           nicknameToIndexTabMap[newNick] = indexTab;
                                           ui->tabWidget->setTabText(indexTab, newNick);
                                       }
                                       break;
                                   }
                               }
                               break;
                           }
                       }
                   }
                   else
                   {
                       for(int i = 0; i < oldNicknames.size(); i++)
                       {
                           if(!nicknames.contains(oldNicknames[i]))
                           {
                               oldNick = oldNicknames[i];
                               indexTab = findIndexTabByNickname(oldNick);
                               if(indexTab != -1)
                               {
                                   removeIndexTabAndNickname(oldNick);
                                   ui->tabWidget->removeTab(indexTab);
                               }
                               break;
                           }
                       }
                   }
                   break;

               case ServerMessage:
                   in >> a;
                   ui->textBrowser->append(a);
                   break;

               case PrivateMessage:
                   in >> nick >>a;
                   if(nicknameToIndexTabMap.empty() or (findIndexTabByNickname(nick) == -1))
                   {
                       privateTab = new PrivateChatForm(nick, this);
                       indexTab = ui->tabWidget->addTab(privateTab, nick);
                       nicknameToIndexTabMap[nick] = indexTab;
                       if(a != "")
                       {
                           if(meFlag)
                           {
                               nick = "You";
                               meFlag = false;
                           }
                           nick += ": " + a;
                           privateTab->printText(nick);
                       }
                   }
                   else
                   {
                       indexTab = findIndexTabByNickname(nick);
                       privateTab = qobject_cast<PrivateChatForm*>(ui->tabWidget->widget(indexTab));
                       if(!meFlag)
                       {
                           nick += ": " + a;
                           privateTab->printText(nick);
                       }
                       else
                       {
                           meFlag = false;
                       }
                   }
                   break;
           }
           blockSize = 0;
       }
    }
    else
    {
        ui->textBrowser->append("*** ОШИБКА ЧТЕНИЯ");
    }
}

void MainWindow::on_inputText_returnPressed()
{
    QStringList list = ui->inputText->text().split(" ");
    QString msg;
    for(int i = 0; i < list.size(); i++)
    {
        if(list[i] != "")
        {
            msg += list[i] + " ";
        }
    }
    list = msg.split(" ");
    if(socket->state() == QTcpSocket::ConnectedState)
    {
        if(list[0] != "/private")
        {
            if(msg != "")
            {
                SendMess(msg);
            }
            else
            {
                ui->inputText->clear();
            }
        }
        else
        {
            meFlag = true;
            if(list.size() > 1)
            {
                if(nicknames.contains(list[1]) and (list[1] != nickname))
                {
                    privateNickname = list[1];
                    if(list.size() > 2)
                    {
                        QString msg = list[2];
                        for(int i = 3; i < list.size(); i++)
                        {
                            msg += " " + list[i];
                        }
                        SendMess(msg, PrivateMessage);
                    }
                    else
                    {
                        SendMess(msg, PrivateMessage);
                    }
                }
                else
                {
                    if(list[1] == nickname)
                    {
                        QMessageBox::warning(this, "Внимание", "С собой чат начать нельзя!");
                    }
                    else
                    {
                        QMessageBox::warning(this, "Внимание", "Данный пользователь не найден!");
                    }
                }
            }
            else
            {
                QMessageBox::warning(this, "Внимание", "Данный пользователь не найден!");
            }
        }
    }
    else
    {
        QMessageBox::warning(this, "Внимание", "Вы не подключены к серверу!");
    }
}

void MainWindow::on_sendButton_clicked()
{
    MainWindow::on_inputText_returnPressed();
}

void MainWindow::on_nickButton_pressed()
{
    if(socket->state() == QTcpSocket::ConnectedState)
    {
        if(nicknames.contains(ui->inputNick->text()))
        {
            QMessageBox::warning(this, "Внимание", "Этот никнейм уже есть у одного из присутствующих на сервере");
            ui->inputNick->clear();
            ui->inputNick->insert(nickname);
        }
        else
        {
            MessageCodes msgCode = nickname.isEmpty() ? NewNicknameMessage : NicknameMessage;
            SendMess(ui->inputNick->text(), msgCode);
            nickname = ui->inputNick->text();
        }
    }
    else
    {
        QMessageBox::warning(this, "Внимание", "Вы не подключены к серверу");
    }
}

void MainWindow::on_help_triggered()
{
    QMessageBox::information(this, "Помощь", "/private <nickname> [message] - начать личный чат [с сообщением]"
                                             "");
}

int MainWindow::findIndexTabByNickname(const QString& nickname)
{
    auto it = nicknameToIndexTabMap.find(nickname);
    if (it != nicknameToIndexTabMap.end())
    {
        return it->second;
    }
    return -1;
}

void MainWindow::removeIndexTabAndNickname(const QString& nickname)
{
    auto nicknameIt = nicknameToIndexTabMap.find(nickname);
    if (nicknameIt != nicknameToIndexTabMap.end())
    {
        nicknameToIndexTabMap.erase(nicknameIt);
    }
}
//bug: someone in private chatting cant receive first message sometimes and also when in public through /private msg dont show for sender
