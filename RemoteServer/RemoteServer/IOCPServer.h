#pragma once
#include <MSWSock.h>
#include <map>
#include <memory>
#include <list>
#include "ThreadPool.h"
#include "Packet.h"
#include <vector>
#include "Command.h"
class IOCPClient;
class IOCPServer;
enum class OverlappedOperator : DWORD {
	ONone,
	OAccept,
	ORecv,
	OSend,
	OError
};
class OverLapped {
public:
	OverLapped() = default;
	OVERLAPPED m_overlapped;
	OverlappedOperator m_operator;
	std::vector<char> m_buffer; //缓冲区
	std::function<int()> m_worker;//处理函数
	IOCPServer* m_server; //服务器对象
	IOCPClient* m_client; //对应的客户端
	WSABUF m_wsabuffer;
	virtual ~OverLapped() { m_buffer.clear(); }
};

class AcceptOverLapped : public OverLapped {
public:
	AcceptOverLapped();
	int AcceptWorker();
};
class RecvOverLapped : public OverLapped {
public:
	RecvOverLapped();
	int RecvWorker();
	CCommand m_cmd;
};
class SendOverLapped : public OverLapped {
public:
	SendOverLapped();
	int SendWorker();
};;

// 管理多个client生命周期，封装IOCP操作
class IOCPServer
{
public:
	IOCPServer(const std::string& ip = "127.0.0.1", short port = 9527);
	~IOCPServer();
	void StartServer();
	bool WaitNewAccept();
	void BindNewAccept(SOCKET socket);
	void AddClient(SOCKET socket, std::shared_ptr<IOCPClient> client);
	void RemoveClient(SOCKET socket);
private:
	void CreateSocket();
	int threadIocp();
private:
	std::map<SOCKET, std::shared_ptr<IOCPClient>> sock2Client;
	ThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_ListenSock;
	sockaddr_in m_addr;
};
class IOCPClient : public std::enable_shared_from_this<IOCPClient> {
public:
	IOCPClient() :m_isbusy(false), m_flags(0),
		m_AccpetOverlapped(new AcceptOverLapped()),
		m_RecvOverlapped(new RecvOverLapped()),
		m_SendOverlapped(new SendOverLapped()) {
		m_ClientSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		m_recvbuffer.resize(1024);
		memset(&m_laddr, 0, sizeof(m_laddr));
		memset(&m_raddr, 0, sizeof(m_raddr));
	}

	~IOCPClient() {
		closesocket(m_ClientSock);
		m_AccpetOverlapped.reset();
		m_RecvOverlapped.reset();
		m_SendOverlapped.reset();
		m_recvbuffer.clear();
	}
	void SetOverlapped(IOCPClient* ptr) {
		m_AccpetOverlapped->m_client = ptr;
		m_RecvOverlapped->m_client = ptr;
		m_SendOverlapped->m_client = ptr;
	}
	// 用于与windowsAPI交互的类型转换函数
	operator SOCKET() { return m_ClientSock; }
	operator PVOID() { return &m_recvbuffer[0]; }
	operator LPOVERLAPPED() { return &m_AccpetOverlapped->m_overlapped; }
	operator LPDWORD() { return &m_received; }

	LPWSABUF RecvWSABuffer() { return &m_RecvOverlapped->m_wsabuffer; }
	LPWSAOVERLAPPED RecvOverlapped() { return &m_RecvOverlapped->m_overlapped; }
	LPWSABUF SendWSABuffer() { return &m_SendOverlapped->m_wsabuffer; }
	LPWSAOVERLAPPED SendOverlapped() { return &m_SendOverlapped->m_overlapped; }
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_recvbuffer.size(); }
	int Recv();


	SOCKET m_ClientSock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<AcceptOverLapped> m_AccpetOverlapped;
	std::shared_ptr<RecvOverLapped>m_RecvOverlapped;
	std::shared_ptr<SendOverLapped>m_SendOverlapped;
	std::list<CPacket> recvPackets;
	std::list<CPacket> sendPackets;
	std::vector<char> m_recvbuffer;
	size_t m_usedbuffer; //已经使用的缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};



