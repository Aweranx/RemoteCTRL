// RemoteServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <iostream>
#include <winsock2.h> // 网络编程头文件
#include <ws2tcpip.h> // IP地址转换辅助

// 自动链接 ws2_32.lib，省去在项目属性里配置
#pragma comment(lib, "ws2_32.lib")

// 唯一的应用程序对象
CWinApp theApp;

using namespace std;

// 一个辅助函数：获取本机 IP (简单的测试用)
void ShowNetworkInfo() {
    WSADATA wsaData;
    // 1. 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup 失败！" << endl;
        return;
    }

    char hostname[256];
    // 2. 获取主机名
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        cout << "--------------------------------" << endl;
        cout << "主机名: " << hostname << endl;

        // 3. 根据主机名获取 IP 信息
        struct addrinfo hints = { 0 }, * res = nullptr;
        hints.ai_family = AF_INET; // 只看 IPv4
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname, nullptr, &hints, &res) == 0) {
            struct addrinfo* ptr = res;
            while (ptr != nullptr) {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
                char ipstr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, INET_ADDRSTRLEN);
                cout << "IP 地址: " << ipstr << endl;
                ptr = ptr->ai_next;
            }
            freeaddrinfo(res);
        }
    }
    else {
        cout << "无法获取主机名" << endl;
    }
    WSACleanup();
    cout << "--------------------------------" << endl;
}

auto main() -> int
{
    // ==========================================
    // 强制开启控制台窗口
    // ==========================================
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout); // 重定向 cout
    freopen_s(&stream, "CONOUT$", "w", stderr); // 重定向 cerr
    // ==========================================
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // ==========================================
            // 测试代码开始
            // ==========================================

            // 1. 测试 MFC CString (验证 MFC 库是否可用)
            CString strAppTitle;
            strAppTitle.Format(_T("RemoteServer 服务端启动测试 - 进程ID: %d"), GetCurrentProcessId());

            // 使用 wprintf 输出宽字符，(LPCTSTR) 是 CString 转 const wchar_t* 的关键
            wprintf(L"[%s]\n", (LPCTSTR)strAppTitle);

            // 2. 测试网络环境 (模拟服务器上线前的检查)
            cout << "正在检查网络环境..." << endl;
            ShowNetworkInfo();

            // 3. 模拟等待连接
            cout << "服务已就绪，准备进入主循环..." << endl;

            // 这里原本是 while(CServerSocket...)
            // 现在我们简单模拟一下
            for (int i = 3; i > 0; i--) {
                cout << "倒计时 " << i << " 秒后退出测试..." << endl;
                Sleep(1000);
            }

            cout << "测试结束，Bye!" << endl;

            // ==========================================
            // 测试代码结束
            // ==========================================
        }
    }
    else
    {
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}