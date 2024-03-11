#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

#include "qxymodem.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit_Send->setPlaceholderText("Enter the file path to send");
    ui->lineEdit_Recv->setPlaceholderText("Enter the file path to receive");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_toolButton_Send_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select a file to send");
    ui->lineEdit_Send->setText(file);
}

void MainWindow::on_toolButton_recv_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, "Select a file to receive");
    ui->lineEdit_Recv->setText(file);
}

void MainWindow::on_pushButton_Start_clicked()
{
    QString send = ui->lineEdit_Send->text();
    QString recv = ui->lineEdit_Recv->text();
    if (send.isEmpty() || recv.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter the file path");
        return;
    }
    if (send == recv) {
        QMessageBox::warning(this, "Error", "The file path to send and receive cannot be the same");
        return;
    }
    ui->lineEdit_Send->setDisabled(true);
    ui->lineEdit_Recv->setDisabled(true);
    ui->toolButton_Send->setDisabled(true);
    ui->toolButton_recv->setDisabled(true);

    QString sendFilePath = send;
    QFileInfo sendFileInfo(sendFilePath);
    QString sendFileName = sendFileInfo.fileName();
    QString recvFilePath = recv;
    QFileInfo recvFileInfo(recvFilePath);
    QString recvFileName = recvFileInfo.fileName();
    QString recvFileDir = recvFileInfo.dir().absolutePath();

    QXmodemFile *s = new QXmodemFile(sendFilePath,this);
    QXmodemFile *r = new QXmodemFile(recvFilePath,this);
    connect(s,&QXmodemFile::send,r,&QXmodemFile::receive);
    connect(r,&QXmodemFile::send,s,&QXmodemFile::receive);
    connect(s,&QXmodemFile::finished,this,[=]{
        ui->lineEdit_Send->setDisabled(false);
        ui->lineEdit_Recv->setDisabled(false);
        ui->toolButton_Send->setDisabled(false);
        ui->toolButton_recv->setDisabled(false);
        s->deleteLater();
    });
    connect(r,&QXmodemFile::finished,this,[=]{
        ui->lineEdit_Send->setDisabled(false);
        ui->lineEdit_Recv->setDisabled(false);
        ui->toolButton_Send->setDisabled(false);
        ui->toolButton_recv->setDisabled(false);
        r->deleteLater();
    });

    s->startSend();
    r->startRecv();
}
