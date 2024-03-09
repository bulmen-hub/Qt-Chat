#include "privatechatform.h"
#include "ui_privatechatform.h"
#include "mainwindow.h"

PrivateChatForm::PrivateChatForm(QString nick, MainWindow *reference, QWidget *parent) : QWidget(parent), ui(new Ui::PrivateChatForm)
{
    ui->setupUi(this);
    nickname = nick;
    mainInstance = reference;
}

PrivateChatForm::~PrivateChatForm()
{
    delete ui;
}

void PrivateChatForm::on_privateButton_clicked()
{
    QString msg = ui->privateInput->text();
    mainInstance->privateNickname = nickname;
    QString ownMsg = "You: " + msg;
    ui->privateTextBrowser->append(ownMsg);
    mainInstance->meFlag = true;
    mainInstance->SendMess(msg, MainWindow::PrivateMessage);
    ui->privateInput->clear();
}

void PrivateChatForm::printText(QString msg)
{
    ui->privateTextBrowser->append(msg);
}

void PrivateChatForm::on_privateInput_returnPressed()
{
    on_privateButton_clicked();
}

