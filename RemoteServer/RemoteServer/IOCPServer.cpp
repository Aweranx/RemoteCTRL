#include "pch.h"
#include "IOCPServer.h"

IOCPServer::IOCPServer(const std::string& ip, short port) : m_pool(ThreadPool(4)) {
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_ListenSock = INVALID_SOCKET;
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

IOCPServer::~IOCPServer()
{
	closesocket(m_ListenSock);
	auto it = sock2Client.begin();
	for (; it != sock2Client.end(); it++) {
		it->second.reset();
	}
	sock2Client.clear();
	CloseHandle(m_hIOCP);
}

void IOCPServer::CreateSocket()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_ListenSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_ListenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

bool IOCPServer::WaitNewAccept() {
	auto pClient = new IOCPClient();
	pClient->SetOverlapped(pClient);
	if (!AcceptEx(m_ListenSock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient))
	{
		TRACE("%d\r\n", WSAGetLastError());
		if (WSAGetLastError() != WSA_IO_PENDING) {
			closesocket(m_ListenSock);
			m_ListenSock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void IOCPServer::BindNewAccept(SOCKET socket) {
	CreateIoCompletionPort((HANDLE)socket, m_hIOCP, (ULONG_PTR)this, 0);
}

void IOCPServer::AddClient(SOCKET socket, std::shared_ptr<IOCPClient> client) {
	sock2Client.insert({ socket, client });
}
void IOCPServer::RemoveClient(SOCKET socket) {
	if(sock2Client.find(socket)==sock2Client.end())
		return;
	closesocket(socket);
	sock2Client.erase(socket);
}

void IOCPServer::StartServer() {
	CreateSocket();
	if (bind(m_ListenSock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_ListenSock);
		m_ListenSock = INVALID_SOCKET;
		return;
	}
	if (listen(m_ListenSock, 3) == -1) {
		closesocket(m_ListenSock);
		m_ListenSock = INVALID_SOCKET;
		return;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) {
		closesocket(m_ListenSock);
		m_ListenSock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return;
	}
	CreateIoCompletionPort((HANDLE)m_ListenSock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.enqueue([this]() {this->threadIocp(); });
	if (!WaitNewAccept()) return;
	// m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	// m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
}

int IOCPServer::threadIocp() {
	DWORD tranferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if (CompletionKey != 0) {
			OverLapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, OverLapped, m_overlapped);
			TRACE("pOverlapped->m_operator %d \r\n", pOverlapped->m_operator);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator) {
			case OverlappedOperator::OAccept:
			{
				AcceptOverLapped* pOver = (AcceptOverLapped*)pOverlapped;
				m_pool.enqueue(pOver->m_worker);
			}
			break;
			case OverlappedOperator::ORecv:
			{
				RecvOverLapped* pOver = (RecvOverLapped*)pOverlapped;
				m_pool.enqueue(pOver->m_worker);
			}
			break;
			case OverlappedOperator::OSend:
			{
				TRACE("Send");
				SendOverLapped* pOver = (SendOverLapped*)pOverlapped;
				m_pool.enqueue(pOver->m_worker);
			}
			break;
			case OverlappedOperator::OError:
			{
				TRACE("Error Overlapped Operator\r\n");
			}
			break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}

int IOCPClient::Recv()
{
	int ret = recv(m_ClientSock, m_recvbuffer.data() + m_usedbuffer, m_recvbuffer.size() - m_usedbuffer, 0);
	if (ret <= 0) return -1;
	m_usedbuffer += (size_t)ret;

	return ret;
}

AcceptOverLapped::AcceptOverLapped() {
	m_worker = [this]() { return this->AcceptWorker(); };
	m_operator = OverlappedOperator::OAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}
int AcceptOverLapped::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		sockaddr* plocal = NULL, * promote = NULL;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength, //本地地址
			(sockaddr**)&promote, &rLength//远程地址
		);
		memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), promote, sizeof(sockaddr_in));
		m_server->BindNewAccept(*m_client);
		m_server->AddClient(*m_client, m_client->shared_from_this());
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
			TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());
		}
		if (!m_server->WaitNewAccept())
		{
			return -2;
		}
	}
	return -1;
}

SendOverLapped::SendOverLapped() {
	m_worker = [this]() { return this->SendWorker(); };
	m_operator = OverlappedOperator::OSend;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

int SendOverLapped::SendWorker() {
	
	while (m_client->sendPackets.size() > 0)
	{

		//TRACE("send size: %d", m_client->sendPackets.size());
		CPacket pack = m_client->sendPackets.front();
		m_client->sendPackets.pop_front();
		int ret = send(m_client->m_ClientSock, pack.Data(), pack.Size(), 0);
		TRACE("send ret: %d\r\n", ret);

	}
	m_server->RemoveClient(m_client->m_ClientSock);
	return -1;
}

RecvOverLapped::RecvOverLapped() {
	m_worker = [this]() { return this->RecvWorker(); };
	m_operator = OverlappedOperator::ORecv;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

int RecvOverLapped::RecvWorker() {
	int index = 0;

	int len = m_client->Recv();
	index += len;
	CPacket pack((BYTE*)m_client->m_recvbuffer.data(), (size_t&)index);
	m_cmd.ExcuteCommand(m_client->sendPackets, pack);

	if (index == 0) {
		WSASend((SOCKET)*m_client, m_client->SendWSABuffer(), 1, *m_client, m_client->flags(), m_client->SendOverlapped(), NULL);
		TRACE("命令: %d\r\n", pack.sCmd);


	}
	return -1;
}