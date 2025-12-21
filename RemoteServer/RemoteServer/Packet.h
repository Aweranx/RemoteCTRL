#pragma once
#include "global.h"
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket();
	CPacket(ControlCmd nCmd, const BYTE* pData, size_t nSize);
	CPacket(ControlCmd nCmd);
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
