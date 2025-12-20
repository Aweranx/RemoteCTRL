#include "pch.h"
#include "TcpServer.h"
#include <list>

TcpServer::TcpServer() : m_command(){}
TcpServer::~TcpServer() {}

void TcpServer::run() {
	CServerSocket& pserver = CServerSocket::GetInstance();
	int count = 0;
	std::list<CPacket> lstPacket;
	while (true) {
		if (pserver.AcceptClient() == false) {
			if (count >= 3) {
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
			count++;
		}
		TRACE("AcceptClient return true\r\n");
		CPacket packet;
		pserver.GetPacket(packet);
		TRACE("DealCommand ret %s\r\n", packet.Data());
		ControlCmd cmd = packet.sCmd;
		TRACE("DealCommand ret %d\r\n", cmd);
		if (static_cast<WORD>(cmd) > 0) {
			int ret = m_command.ExcuteCommand(lstPacket, packet);
			if (ret != 0) {
				TRACE("执行命令失败：%d ret=%d\r\n", cmd, ret);
			}
			TRACE("Command has done!\r\n");
		}
		while (lstPacket.size() > 0) {
			CPacket& outPacket = lstPacket.front();
			pserver.Send(outPacket);
			lstPacket.pop_front();
		}
		pserver.CloseClient();
	}
}
