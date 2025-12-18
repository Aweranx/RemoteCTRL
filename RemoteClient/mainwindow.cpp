#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ipaddressedit.h"
#include <QPushButton>
#include <QTreeWidget>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

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
    ui->fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->fileList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onFileListContextMenu);


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
        if (fileName == "." || fileName == ".." || finfo.HasNext == false) {
            return;
        }
        QString parentPath = m_currentItem->data(0, Qt::UserRole).toString();
        QString fullPath;
        if (parentPath.endsWith('/') || parentPath.endsWith('\\')) {
            fullPath = parentPath + fileName;
        } else {
            fullPath = parentPath + "/" + fileName;
        }
        if(finfo.IsDirectory > 0) {
            if (m_currentItem == nullptr) return; //以此防崩溃

            // 构造函数传入父节点指针，会自动 addChild
            QTreeWidgetItem *item = new QTreeWidgetItem(m_currentItem);
            item->setText(0, fileName);
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
        } else {
            QListWidgetItem *item = new QListWidgetItem(ui->fileList);
            item->setText(fileName);
            item->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
            item->setData(Qt::UserRole, fullPath);
            // 存储类型标记 (0 或 false 表示不是目录)
            item->setData(Qt::UserRole + 1, false);
        }
    };
    m_handler[3] = [this](CPacket& packet){
        qDebug() << "运行";
    };
    m_handler[4] = [this](CPacket& packet){
        QByteArray payload = packet.strData;
        // 收到空包 (Server: CPacket pack(4, NULL, 0))
        if (payload.size() == 0) {
            if (m_downloadFile.isOpen()) {
                m_downloadFile.close();
                m_isDownloading = false;
                QMessageBox::information(this, "提示", "下载完成！");
                qDebug() << "文件接收完毕，实际接收大小:" << m_receivedSize;
            }
            return;
        }
        // 第一个包只传来头
        // 收到文件大小头 (Server: 发送 8字节 long long)
        // 处于下载模式 且 文件还没打开
        if (m_isDownloading && !m_downloadFile.isOpen()) {
            if (payload.size() == 8) {
                qint64 serverSize = 0;
                // 从 QByteArray 中取出 long long (8字节)
                memcpy(&serverSize, payload.constData(), 8);

                // 服务端逻辑：如果是 0，代表 fopen 失败
                if (serverSize == 0) {
                    QMessageBox::warning(this, "错误", "服务器无法打开该文件（可能被占用或不存在）");
                    m_isDownloading = false;
                    return;
                }
                qint64 m_totalSize = serverSize;
                m_receivedSize = 0;
                if (!m_downloadFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    QMessageBox::warning(this, "错误", "无法打开本地文件进行写入：" + m_downloadFile.fileName());
                    m_isDownloading = false;
                    return;
                }
                qDebug() << "收到头部，文件大小:" << m_totalSize << " 开始下载...";
            } else {
                qDebug() << "错误：期待收到8字节文件头，但收到了" << payload.size() << "字节";
            }
            return;
        }

        // 收到文件内容数据
        if (m_downloadFile.isOpen()) {
            qint64 len = m_downloadFile.write(payload);
            if (len != -1) {
                m_receivedSize += len;
            }
        }
    };

    m_handler[9] = [this](CPacket& packet){
        qDebug() << "删除";
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
    ui->fileList->clear();

    CPacket pack(2, path.toUtf8());
    m_tcpclient->sendPacket(pack);
}

void MainWindow::onFileListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->fileList->itemAt(pos);
    if (item == nullptr) {
        return;
    }

    QMenu menu(this);
    QAction *pDeleteAction = menu.addAction("删除");
    pDeleteAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    QAction *pRunAction = menu.addAction("运行");
    pRunAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    QAction *pDownloadAction = menu.addAction("下载");
    pDownloadAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));

    QAction *pSelectedAction = menu.exec(QCursor::pos());
    QString fullPath = item->data(Qt::UserRole).toString();
    if (fullPath.isEmpty()) return;
    if (pSelectedAction == pDeleteAction) {
        int ret = QMessageBox::question(this, "确认删除",
                                        "确定要删除文件吗？\n" + fullPath,
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        CPacket pack(9, fullPath.toLocal8Bit());
        m_tcpclient->sendPacket(pack);
        int row = ui->fileList->row(item);
        delete ui->fileList->takeItem(row);
    } else if(pSelectedAction == pRunAction) {
        CPacket pack(3, fullPath.toLocal8Bit());
        m_tcpclient->sendPacket(pack);
    } else if(pSelectedAction == pDownloadAction) {
        QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QString fileName = item->text();
        QString localPath = downloadDir + "/" + fileName;
        localPath = QDir::toNativeSeparators(localPath);
        qDebug() << "准备下载文件，保存路径为:" << localPath;
        if (m_downloadFile.isOpen()) {
            m_downloadFile.close();
        }
        m_downloadFile.setFileName(localPath);
        m_isDownloading = true;
        CPacket pack(4, fullPath.toLocal8Bit());
        m_tcpclient->sendPacket(pack);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
