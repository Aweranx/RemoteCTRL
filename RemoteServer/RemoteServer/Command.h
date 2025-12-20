#pragma once
#include "LockDialog.h"
#include <map>
#include <list>
#include "global.h"

class CPacket;
class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommand(std::list<CPacket>& lstPacket, CPacket& inPacket);
	
private:
	using CommandHandler = int (CCommand::*)(std::list<CPacket>&, CPacket&);
	std::map<ControlCmd, CommandHandler> m_cmdHandlers;
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket);
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	//unsigned __stdcall threadLockDlg(void* arg);
};

