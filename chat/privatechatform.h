#ifndef PRIVATECHATFORM_H
#define PRIVATECHATFORM_H

#include <QWidget>
#include "mainwindow.h"

namespace Ui {
class PrivateChatForm;
}

class PrivateChatForm : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateChatForm(QString nick, MainWindow *reference, QWidget *parent = 0);
    ~PrivateChatForm();
    void printText(QString msg);
    QString nickname;

private slots:
    void on_privateButton_clicked();

    void on_privateInput_returnPressed();

private:
    Ui::PrivateChatForm *ui;
    MainWindow *mainInstance;
};

#endif // PRIVATECHATFORM_H
