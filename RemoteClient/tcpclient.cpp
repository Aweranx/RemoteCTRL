#include "tcpclient.h"
#include <QDebug>

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

CPacket::CPacket(quint16 nCmd, const QByteArray& pData) {
    sHead = PACKET_HEAD;
    sCmd = nCmd;
    strData = pData;
    // Length = sizeof(sCmd) + DataSize + sizeof(sSum)
    nLength = sizeof(quint16) + strData.size() + sizeof(quint16);
    sSum = 0;
    // 计算校验和
    for (const char byte : strData) {
        sSum += static_cast<quint8>(byte);
    }
}

CPacket::CPacket(const QByteArray& rawData) {
    QDataStream stream(rawData);
    stream.setByteOrder(QDataStream::LittleEndian); // 必须设置小端序

    stream >> sHead;
    if (sHead != 0xFEFF) {
        return;
    }
    stream >> nLength;
    stream >> sCmd;
    // nLength = Cmd(2) + Data(n) + Sum(2)
    int dataSize = nLength - 4;
    if (dataSize > 0) {
        char *buffer = new char[dataSize];
        stream.readRawData(buffer, dataSize);
        strData = QByteArray(buffer, dataSize);
        delete[] buffer;
    }
    stream >> sSum;
}

CPacket::CPacket(const CPacket& pack) {
    sHead = pack.sHead;
    nLength = pack.nLength;
    sCmd = pack.sCmd;
    strData = pack.strData;
    sSum = pack.sSum;
}

CPacket& CPacket::operator=(const CPacket& pack) {
    if (this != &pack) {
        sHead = pack.sHead;
        nLength = pack.nLength;
        sCmd = pack.sCmd;
        strData = pack.strData;
        sSum = pack.sSum;
    }
    return *this;
}

QByteArray CPacket::toByteArray() const
{
    QByteArray block;
    QDataStream stream(&block, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian); // Windows/x86 字节序

    stream << sHead;
    stream << nLength;
    stream << sCmd;
    if (!strData.isEmpty()) {
        stream.writeRawData(strData.constData(), strData.size());
    }
    stream << sSum;
    return block;
}

int CPacket::size() const  {
    return nLength + 6;
}


TcpClient::TcpClient(const QString& ip, const quint16 port, QObject *parent)
    : QObject{parent}, m_socket(new QTcpSocket(this)), m_ip(ip), m_port(port) {
    // 连接成功信号
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    // 有新数据可读信号
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    // 连接断开信号
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    // 错误处理信号
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onErrorOccurred);
}

void TcpClient::updateInfo(const QString& ip, const quint16 port) {
    m_ip = ip;
    m_port = port;
}

void TcpClient::sendPacket(const CPacket& packet) {
    connectToServer();
    QByteArray sendData = packet.toByteArray();

    if (sendData.isEmpty()) {
        qDebug() << "[TcpClient] 警告: 尝试发送空数据包";
        return;
    }
    // 写入缓冲区
    qint64 bytesWritten = m_socket->write(sendData);

    if (bytesWritten == -1) {
        qDebug() << "[TcpClient] 发送出错:" << m_socket->errorString();
    }
    else if (bytesWritten != sendData.size()) {
        qDebug() << "[TcpClient] 警告: 数据未完全写入 (实际:" << bytesWritten << "/ 总:" << sendData.size() << ")";
    }
    else {
        // 强制刷新缓冲区，要求立即发送数据
        // Qt 默认是异步发送，flush() 会阻塞直到数据写入底层 socket
        // m_socket->flush();

        qDebug() << "[TcpClient] 发送成功 | 命令:" << QString::number(packet.sCmd, 16).toUpper()
                 << " | 大小:" << bytesWritten << "字节";
    }
}

void TcpClient::connectToServer() {
    m_socket->abort();
    qDebug() << "正在连接到:" << m_ip << ":" << m_port;
    m_socket->connectToHost(m_ip, m_port);
}

void TcpClient::onConnected() {
    qDebug() << "连接服务器成功！";
}

void TcpClient::onReadyRead()
{
    QByteArray temp = m_socket->readAll();
    m_buffer.append(temp);

    // 判断头是否读完 -> 校验head -> 比较读到的数据size和nLength
    // 不够则继续等待 -> 够了提取出来CPacket发送信号
    while (m_buffer.size() >= 6) {
        QDataStream stream(m_buffer);
        stream.setByteOrder(QDataStream::LittleEndian);
        quint16 head;
        quint32 bodyLen;
        stream >> head >> bodyLen;

        if (head != PACKET_HEAD) {
            m_buffer.remove(0, 1);
            continue;
        }

        // 总大小 = Head(2) + Length字段本身(4) + nLength(Cmd+Data+Sum)
        int totalPacketSize = 2 + 4 + bodyLen;
        // 检查缓冲区数据是否足够一个完整的包
        if (m_buffer.size() < totalPacketSize) {
            break;
        }
        // 数据足够，提取这一个完整的包
        QByteArray packetData = m_buffer.left(totalPacketSize);
        m_buffer.remove(0, totalPacketSize);
        CPacket pack(packetData);
        if (pack.sHead == PACKET_HEAD) {
            qDebug() << "成功解析完整包 | Cmd:" << pack.sCmd << " 数据大小:" << pack.strData.size();
            emit recvPacket(pack);
        } else {
            qDebug() << "解析后的包头校验失败";
        }
    }
}

void TcpClient::onDisconnected()
{
    qDebug() << "与服务器断开连接。";
}

void TcpClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    // 对于短连接协议来说，远程主机关闭连接意味着传输完成
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        qDebug() << "[TcpClient] 服务器已断开连接";
        return;
    }
    // 连接超时、网络断开等才打印 Error
    qDebug() << "[TcpClient] 发生错误:" << m_socket->errorString() << "代码:" << socketError;
}

TcpClient::~TcpClient() {
    if(m_socket->isOpen()) {
        m_socket->close();
    }
}
