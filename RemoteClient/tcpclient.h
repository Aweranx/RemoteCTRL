#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QPoint>
constexpr quint16 PACKET_HEAD = 0xFEFF;
enum class ControlCmd : quint16 {
    Invalid = 0,
    TestConnect = 0x07BD, // 1981: 测试连接

    // 磁盘与文件
    GetDisk = 1,
    GetFiles = 2,
    RunFile = 3,
    DelFile = 4,
    DownloadFile = 5,

    // 鼠标与屏幕
    MouseEvent = 6,
    ScreenSpy = 7,

    // 系统控制
    LockMachine = 8,
    UnlockMachine = 9
};
class FILEINFO {
public:
    FILEINFO() {
        IsInvalid = false;
        IsDirectory = -1;
        HasNext = true;
        memset(szFileName, 0, sizeof(szFileName));
    }
    int32_t IsInvalid;//是否有效
    int32_t IsDirectory;//是否为目录 0 否 1 是
    int32_t HasNext;//是否还有后续 0 没有 1 有
    char szFileName[256];//文件名
};
#pragma pack(push, 1)
class MOUSEEV {
public:
    MOUSEEV() {
        nAction = 0;
        nButton = 0xFFFF;
        ptXY.x = 0;
        ptXY.y = 0;
    }

    MOUSEEV(quint16 action, quint16 button, QPoint point) {
        nAction = action;
        nButton = button;
        ptXY.x = point.x();
        ptXY.y = point.y();
    }

    quint16 nAction; // 点击、移动、双击
    quint16 nButton; // 左键、右键、中键

    struct {
        qint32 x;
        qint32 y;
    } ptXY;
    QPoint toQPoint() const {
        return QPoint(ptXY.x, ptXY.y);
    }
};

#pragma pack(pop)

class CPacket
{
public:
    CPacket();
    CPacket(ControlCmd nCmd, const QByteArray& pData);
    CPacket(const QByteArray& rawData);
    CPacket(const CPacket& pack);
    CPacket& operator=(const CPacket& pack);
    ~CPacket() = default;

    // 获取序列化后的完整二进制数据
    QByteArray toByteArray() const;

    int size() const;

public:
    quint16 sHead;      // 固定位 0xFEFF
    quint32 nLength;    // nLength = sizeof(sCmd) + DataSize + sizeof(sSum)
    ControlCmd sCmd;       // 控制命令
    QByteArray strData; // 包内容
    quint16 sSum;       // 和校验
};

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(const QString& ip, const quint16 port, QObject *parent = nullptr);
    ~TcpClient();
    void sendPacket(const CPacket& packet);
    void updateInfo(const QString& ip, const quint16 port);


signals:
    void recvPacket(CPacket& packet);

private slots:
    // 处理连接成功
    void onConnected();
    // 处理收到数据
    void onReadyRead();
    // 处理断开连接
    void onDisconnected();
    // 处理错误
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    void connectToServer();
    QTcpSocket* m_socket;
    QString m_ip;
    quint16 m_port;
    CPacket m_packet;
    QByteArray m_buffer;
};

#endif // TCPCLIENT_H
