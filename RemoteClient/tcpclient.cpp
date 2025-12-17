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
    // if (m_socket->state() != QAbstractSocket::ConnectedState) {
    //     qDebug() << "[TcpClient] 发送失败: 未连接到服务器";
    //     return;
    // }
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
    QByteArray data = m_socket->readAll();

    // 打印十六进制数据，方便查看每个字节
    // qDebug() << "收到原始数据(Hex):" << data.toHex(' ').toUpper();

    // 尝试解析
    m_packet = CPacket(data);
    qDebug() << "解析收到命令:" << m_packet.sCmd << " 长度:" << m_packet.nLength;
    emit recvPacket(m_packet);
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
