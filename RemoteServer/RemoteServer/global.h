#pragma once
#include <wtypes.h>

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