#pragma once
#include "pch.h"
#include "framework.h"
enum class ControlCmd : WORD {
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
class MOUSEEV {
public:
	MOUSEEV() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、中键
	POINT ptXY;//坐标
};

class FILEINFO {
public:
	FILEINFO() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录 0 否 1 是
	BOOL HasNext;//是否还有后续 0 没有 1 有
	char szFileName[256];//文件名
};

#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket();
	CPacket(ControlCmd nCmd, const BYTE* pData, size_t nSize);
	CPacket(const CPacket& pack);
	CPacket(const BYTE* pData, size_t& nSize);
	~CPacket();
	CPacket& operator=(const CPacket& pack);
	int Size();
	const char* Data();
	bool GetFilePath(std::string& strPath);
	bool GetMouseEvent(MOUSEEV& mouse);
public:
	WORD sHead;//固定位 0xFEFF
	DWORD nLength;//包长度（从控制命令开始，到和校验结束）
	ControlCmd sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	std::string strOut;//整个包的数据
};
#pragma pack(pop)



class CServerSocket
{
public:
	static CServerSocket& GetInstance();
	~CServerSocket();
	bool AcceptClient();
	void CloseClient();
	int GetPacket(CPacket& packet);

	bool Send(const char* pData, int nSize);
	bool Send(CPacket& pack);

private:
	SOCKET m_client;
	SOCKET m_sock;
	CServerSocket();
	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;
	BOOL InitSockEnv();

	
};

