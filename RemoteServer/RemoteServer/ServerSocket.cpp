#include "pch.h"
#include "ServerSocket.h"

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;
	if (nSize > 0) {
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else {
		strData.clear();
	}
	sSum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sSum += BYTE(strData[j]) & 0xFF;
	}
}
CPacket::CPacket(const CPacket& pack) {
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}
CPacket::CPacket(const BYTE* pData, size_t& nSize) {
	size_t i = 0;
	for (; i < nSize; i++) {
		if (*(WORD*)(pData + i) == 0xFEFF) {
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}
	if (i + 4 + 2 + 2 > nSize) {//包数据可能不全，或者包头未能全部接收到
		nSize = 0;
		return;
	}
	nLength = *(DWORD*)(pData + i); i += 4;
	if (nLength + i > nSize) {//包未完全接收到，就返回，解析失败
		nSize = 0;
		return;
	}
	sCmd = *(WORD*)(pData + i); i += 2;
	if (nLength > 4) {
		strData.resize(nLength - 2 - 2);
		memcpy((void*)strData.c_str(), pData + i, nLength - 4);
		i += nLength - 4;
	}
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[j]) & 0xFF;
	}
	if (sum == sSum) {
		nSize = i;//head2 length4 data...
		return;
	}
	nSize = 0;
}
CPacket::~CPacket() {}
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
int CPacket::Size() {//包数据的大小
	return nLength + 6;
}
const char* CPacket::Data() {
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead; pData += 2;
	*(DWORD*)(pData) = nLength; pData += 4;
	*(WORD*)pData = sCmd; pData += 2;
	memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
	*(WORD*)pData = sSum;
	return strOut.c_str();
}

bool CPacket::GetFilePath(std::string& strPath) {
	if (((sCmd >= 2) && (sCmd <= 4)) ||
		(sCmd == 9))
	{
		strPath = strData;
		return true;
	}
	return false;
}
bool CPacket::GetMouseEvent(MOUSEEV& mouse) {
	if (sCmd == 5) {
		memcpy(&mouse, strData.c_str(), sizeof(MOUSEEV));
		return true;
	}
	return false;
}

CServerSocket& CServerSocket::GetInstance() {
	static CServerSocket instance;
	return instance;
}

CServerSocket::CServerSocket() {
	m_client = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(9527);
	if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	if (listen(m_sock, 1) == -1) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
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