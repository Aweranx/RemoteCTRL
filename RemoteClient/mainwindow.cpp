#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_tcpclient = new TcpClient(this);
    m_tcpclient->connectToServer("127.0.0.1", 9527);
}

MainWindow::~MainWindow()
{
    delete ui;
}
