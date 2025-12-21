#pragma once
#include "pch.h"
#include "framework.h"
#include "global.h"
#include "Packet.h"


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

