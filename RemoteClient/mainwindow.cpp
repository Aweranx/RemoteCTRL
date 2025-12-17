#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ipaddressedit.h"
#include <QPushButton>
#include <QTreeWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->ipEdit->setIP("127.0.0.1");
    ui->portEdit->setText("9527");
    ui->fileTree->setHeaderHidden(true);
    initHandler();
    m_tcpclient = new TcpClient(ui->ipEdit->getIP(), ui->portEdit->text().toUShort(), this);

    connect(ui->testConnectBtn, &QPushButton::clicked, this, &MainWindow::testConnect);
    connect(ui->fileBtn, &QPushButton::clicked, this , &MainWindow::checkDriverInfo);
    connect(m_tcpclient, &TcpClient::recvPacket, this, &MainWindow::dealCmd);
    connect(ui->fileTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onFileTreeItemDoubleClicked);


}

void MainWindow::testConnect() {
    CPacket packet(1981, NULL);
    m_tcpclient->sendPacket(packet);
}

void MainWindow::checkDriverInfo() {
    CPacket packet(1, NULL);
    m_tcpclient->sendPacket(packet);
}

void MainWindow::dealCmd(CPacket& packet) {
    m_handler[packet.sCmd](packet);
}

void MainWindow::initHandler() {
    m_handler[1981] =  [](CPacket& packet){
        qDebug() << "测试收到";
    };
    m_handler[1] = [this](CPacket& packet){
        QByteArray payload = packet.strData;
        QString diskStr = QString::fromUtf8(payload);
        ui->fileTree->clear();
        if (!diskStr.isEmpty()) {
            // 按逗号切割成列表
            QStringList diskList = diskStr.split(',');
            for (const QString &disk : diskList) {
                qDebug() << "发现盘符:" << disk;
                // 创建一个新的树节点，父对象设为 fileTree 表示它是顶层节点
                QTreeWidgetItem *item = new QTreeWidgetItem(ui->fileTree);
                item->setText(0, disk + ":");
                // 使用 Qt 内置的磁盘图标
                item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon));
                // 将纯盘符字符串（如 "C"）存入 UserRole，方便后续双击事件获取
                item->setData(0, Qt::UserRole, disk + ":/");
            }
        } else {
            qDebug() << "没有检测到任何磁盘";
        }
    };
    m_handler[2] = [this](CPacket& packet){
        QByteArray payload = packet.strData;
        if (payload.size() < (int)sizeof(FILEINFO)) {
            qDebug() << "数据包长度不足，无法解析 FILEINFO";
            return;
        }
        FILEINFO finfo;
        memcpy(&finfo, payload.constData(), sizeof(FILEINFO));
        QString fileName = QString::fromLocal8Bit(finfo.szFileName);

        qDebug() << "收到文件:" << fileName;

        if (m_currentItem == nullptr) return; //以此防崩溃

        // 构造函数传入父节点指针，会自动 addChild
        QTreeWidgetItem *item = new QTreeWidgetItem(m_currentItem);
        item->setText(0, fileName);

        QString parentPath = m_currentItem->data(0, Qt::UserRole).toString();
        QString fullPath;
        if (parentPath.endsWith('/') || parentPath.endsWith('\\')) {
            fullPath = parentPath + fileName;
        } else {
            fullPath = parentPath + "/" + fileName;
        }
        item->setData(0, Qt::UserRole, fullPath);
        if (finfo.IsDirectory) {
            item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
            // 标记该节点也是目录，可以再次展开
            item->setData(0, Qt::UserRole + 1, true);
        } else {
            item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
            item->setData(0, Qt::UserRole + 1, false);
        }
        m_currentItem->setExpanded(true);
    };
}

void MainWindow::onFileTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) {
        return;
    }
    qDebug() << "双击了路径:" << path;
    m_currentItem = item;
    qDeleteAll(item->takeChildren());

    CPacket pack(2, path.toUtf8());
    m_tcpclient->sendPacket(pack);
}

MainWindow::~MainWindow()
{
    delete ui;
}
