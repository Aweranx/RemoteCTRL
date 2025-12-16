#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ipaddressedit.h"
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->ipEdit->setIP("127.0.0.1");
    ui->portEdit->setText("9527");
    m_tcpclient = new TcpClient(ui->ipEdit->getIP(), ui->portEdit->text().toUShort(), this);

    connect(ui->testConnectBtn, &QPushButton::clicked, this, &MainWindow::testConnect);


}

void MainWindow::testConnect() {
    CPacket packet(1981, NULL);
    m_tcpclient->sendPacket(packet);
}

MainWindow::~MainWindow()
{
    delete ui;
}
