#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();

    void on_inputText_returnPressed();

    void on_sendButton_clicked();

    void on_nickButton_pressed();

    void on_help_triggered();

private:
    QTcpSocket *socket;
    QByteArray Message;
    QString nickname;
    QVector <QString> nicknames;
    void removeNicknameAndIndex(const QString& nickname);
    int findIndexTabByNickname(const QString& nickname);
    void removeIndexTabAndNickname(const QString& nickname);
    quint16 blockSize;

public:
    Ui::MainWindow *ui;
    enum MessageCodes
    {
        RegularMessage, NicknameMessage, NewNicknameMessage, OnlineListMessage, ServerMessage, PrivateMessage, NewPrivateMessage
    };
    QString privateNickname;
    bool meFlag;
    void SendMess(QString a, MessageCodes msgCode = RegularMessage);

public slots:
    void readyRead();

};
#endif // MAINWINDOW_H
