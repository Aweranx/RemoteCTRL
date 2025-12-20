#pragma once
#include "Command.h"
#include "ServerSocket.h"
class TcpServer
{
public:
	TcpServer();
	~TcpServer();
	void run();

private:
	CCommand m_command;
};

