#include "pch.h"
#include "ServerSocket.h"
#include "Command.h"


CServerSocket& CServerSocket::GetInstance() {
	static CServerSocket instance;
	return instance;
}

CServerSocket::CServerSocket() {
	m_client = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！1"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！2"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(9527);
	if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！3"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	if (listen(m_sock, 1) == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！4"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
}

bool CServerSocket::AcceptClient() {
	TRACE("enter AcceptClient\r\n");
	sockaddr_in client_adr;
	int client_sz = sizeof(client_adr);
	m_client = accept(m_sock, (sockaddr*)&client_adr, &client_sz);
	TRACE("m_client = %d\r\n", m_client);
	if (m_client == -1)return false;
	return true;
}

void CServerSocket::CloseClient() {
	closesocket(m_client);
	m_client = INVALID_SOCKET;
}

#define BUFFER_SIZE 4096
int CServerSocket::GetPacket(CPacket& packet) {
	if (m_client == -1)return -1;
	//char buffer[1024] = "";
	char* buffer = new char[BUFFER_SIZE];
	if (buffer == NULL) {
		TRACE("内存不足！\r\n");
		return -2;
	}
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true) {
		size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
		if (len <= 0) {
			delete[]buffer;
			return -1;
		}
		TRACE("recv %d\r\n", len);
		index += len;
		len = index;
		packet = CPacket((BYTE*)buffer, len);
		if (len > 0) {
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			delete[]buffer;
			return 0;
		}
	}
	delete[]buffer;
	return -1;
}

bool CServerSocket::Send(const char* pData, int nSize) {
	if (m_client == -1)return false;
	return send(m_client, pData, nSize, 0) > 0;
}
bool CServerSocket::Send(CPacket& pack) {
	if (m_client == -1)return false;
	//Dump((BYTE*)pack.Data(), pack.Size());
	return send(m_client, pack.Data(), pack.Size(), 0) > 0;
}

CServerSocket::~CServerSocket() {
	closesocket(m_sock);
	WSACleanup();
}

BOOL CServerSocket::InitSockEnv() {
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}
	return TRUE;
}