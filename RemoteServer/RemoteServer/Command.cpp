#include "pch.h"
#include "framework.h"
#include "RemoteServer.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "Command.h"

CCommand::CCommand() {
	m_cmdHandlers[ControlCmd::GetDisk] = &CCommand::MakeDriverInfo;
	m_cmdHandlers[ControlCmd::GetFiles] = &CCommand::MakeDirectoryInfo;
	m_cmdHandlers[ControlCmd::RunFile] = &CCommand::RunFile;
	m_cmdHandlers[ControlCmd::DownloadFile] = &CCommand::DownloadFile;
	m_cmdHandlers[ControlCmd::MouseEvent] = &CCommand::MouseEvent;
	m_cmdHandlers[ControlCmd::ScreenSpy] = &CCommand::SendScreen;
	m_cmdHandlers[ControlCmd::LockMachine] = &CCommand::LockMachine;
	m_cmdHandlers[ControlCmd::UnlockMachine] = &CCommand::UnlockMachine;
	m_cmdHandlers[ControlCmd::TestConnect] = &CCommand::TestConnect;
	m_cmdHandlers[ControlCmd::DelFile] = &CCommand::DeleteLocalFile;
}
CCommand::~CCommand() {
	m_cmdHandlers.clear();
}

int CCommand::ExcuteCommand(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	int ret = 0;
	if (m_cmdHandlers.find(inPacket.sCmd) != m_cmdHandlers.end()) {
		CommandHandler handler = m_cmdHandlers[inPacket.sCmd];
		ret = (this->*handler)(lstPacket, inPacket);
	}
	else {
		TRACE("无法识别的命令：%d\r\n", (WORD)inPacket.sCmd);
		ret = -1;
	}
	return ret;
}

int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {//1==>A 2==>B 3==>C ... 26==>Z
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}
	lstPacket.emplace_back(ControlCmd::GetDisk, (BYTE*)result.c_str(), result.size());
	return 0;
}

int CCommand::MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath;
	//std::list<FILEINFO> lstFileInfos;
	if (inPacket.GetFilePath(strPath) == false) {
		OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！！"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.emplace_back(ControlCmd::GetFiles, (BYTE*)&finfo, sizeof(finfo));
		OutputDebugString(_T("没有权限访问目录！！"));
		return -2;
	}
	_finddata_t fdata;
	intptr_t  hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("没有找到任何文件！！"));
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.emplace_back(ControlCmd::GetFiles, (BYTE*)&finfo, sizeof(finfo));
		return -3;
	}
	int count = 0;
	do {
		FILEINFO finfo;
		memset(&finfo, 0, sizeof(finfo));
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("file : %s\r\n", finfo.szFileName);
		finfo.HasNext = TRUE;
		lstPacket.emplace_back(ControlCmd::GetFiles, (BYTE*)&finfo, sizeof(finfo));
		count++;
	} while (!_findnext(hfind, &fdata));
	TRACE("server: count = %d\r\n", count);
	//发送信息到控制端
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	lstPacket.emplace_back(ControlCmd::GetFiles, (BYTE*)&finfo, sizeof(finfo));
	_findclose(hfind);
	return 0;
}

int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath;
	inPacket.GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	lstPacket.emplace_back(ControlCmd::RunFile);
	return 0;
}
#pragma warning(disable:4966) // fopen sprintf strcpy strstr 
int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath;
	inPacket.GetFilePath(strPath);
	long long data = 0;
	FILE* pFile = NULL;
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0) {
		lstPacket.emplace_back(ControlCmd::DownloadFile, (BYTE*)&data, 8);
		return -1;
	}
	if (pFile != NULL) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		lstPacket.emplace_back(ControlCmd::DownloadFile, (BYTE*)&data, 8);
		fseek(pFile, 0, SEEK_SET);
		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			lstPacket.emplace_back(ControlCmd::DownloadFile, (BYTE*)buffer, rlen);
		} while (rlen >= 1024);
		fclose(pFile);
	}
	lstPacket.emplace_back(ControlCmd::DownloadFile);
	return 0;
}

int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	MOUSEEV mouse;
	if (inPacket.GetMouseEvent(mouse)) {
		DWORD nFlags = 0;
		switch (mouse.nButton) {
		case 0://左键
			nFlags = 1;
			break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键
			nFlags = 4;
			break;
		case 4://没有按键
			nFlags = 8;
			break;
		}
		if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0://单击
			nFlags |= 0x10;
			break;
		case 1://双击
			nFlags |= 0x20;
			break;
		case 2://按下
			nFlags |= 0x40;
			break;
		case 3://放开
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags)
		{
		case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11://左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://单纯的鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		lstPacket.emplace_back(ControlCmd::MouseEvent);
	}
	else {
		OutputDebugString(_T("获取鼠标操作参数失败！！"));
		return -1;
	}
	return 0;
}

int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	CImage screen;//GDI
	HDC hScreen = ::GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//24   ARGB8888 32bit RGB888 24bit RGB565  RGB444
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreen);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMem == NULL)return -1;
	IStream* pStream = NULL;
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatPNG);
		LARGE_INTEGER bg = { 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE pData = (PBYTE)GlobalLock(hMem);
		SIZE_T nSize = GlobalSize(hMem);
		lstPacket.emplace_back(ControlCmd::ScreenSpy, pData, nSize);
		GlobalUnlock(hMem);
	}
	//screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
	/*
	TRACE("png %d\r\n", GetTickCount64() - tick);
	for (int i = 0; i < 10; i++) {
		DWORD tick = GetTickCount64();
		screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
		TRACE("png %d\r\n", GetTickCount64() - tick);
		tick = GetTickCount64();
		screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
		TRACE("jpg %d\r\n", GetTickCount64() - tick) ;
	}*/
	pStream->Release();
	GlobalFree(hMem);
	screen.ReleaseDC();
	return 0;
}
CLockDialog dlg;
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg)
{
	TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
	dlg.Create(IDD_DIALOG_INFO, NULL);
	dlg.ShowWindow(SW_SHOW);
	//遮蔽后台窗口
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//w1
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rect.bottom = LONG(rect.bottom * 1.10);
	TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
	dlg.MoveWindow(rect);
	CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
	if (pText) {
		CRect rtText;
		pText->GetWindowRect(rtText);
		int nWidth = rtText.Width();//w0
		int x = (rect.right - nWidth) / 2;
		int nHeight = rtText.Height();
		int y = (rect.bottom - nHeight) / 2;
		pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
	}

	//窗口置顶
	//dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	//限制鼠标功能
	//ShowCursor(false);
	//隐藏任务栏
	//::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
	//限制鼠标活动范围
	dlg.GetWindowRect(rect);
	rect.left = 0;
	rect.top = 0;
	rect.right = 1;
	rect.bottom = 1;
	ClipCursor(rect);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x41) {//按下a键 退出  ESC（1B)
				break;
			}
		}
	}
	ClipCursor(NULL);
	//恢复鼠标
	ShowCursor(true);
	//恢复任务栏
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
	dlg.DestroyWindow();
	_endthreadex(0);
	return 0;
}

int CCommand::LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
		//_beginthread(threadLockDlg, 0, NULL);
		_beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
		TRACE("threadid=%d\r\n", threadid);
	}
	lstPacket.emplace_back(ControlCmd::LockMachine);
	return 0;
}

int CCommand::UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
	//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
	PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
	lstPacket.emplace_back(ControlCmd::UnlockMachine);
	return 0;
}

int CCommand::TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	lstPacket.emplace_back(ControlCmd::TestConnect);
	return 0;
}

int CCommand::DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string strPath;
	inPacket.GetFilePath(strPath);
	TCHAR sPath[MAX_PATH] = _T("");
	//mbstowcs(sPath, strPath.c_str(), strPath.size()); //中文容易乱码
	MultiByteToWideChar(
		CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
		sizeof(sPath) / sizeof(TCHAR));
	DeleteFileA(strPath.c_str());
	lstPacket.emplace_back(ControlCmd::DelFile);
	return 0;
}

