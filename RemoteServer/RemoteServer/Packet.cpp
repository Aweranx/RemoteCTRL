#include "pch.h"
#include "Packet.h"

CPacket::CPacket() :sHead(0), nLength(0), sCmd(ControlCmd::Invalid), sSum(0) {}
CPacket::CPacket(ControlCmd nCmd, const BYTE* pData, size_t nSize) {
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
CPacket::CPacket(ControlCmd nCmd) {
	sHead = 0xFEFF;
	sCmd = nCmd;
	strData.clear();
	nLength = sizeof(WORD) + sizeof(WORD); // Cmd + Sum
	sSum = 0;
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
	sCmd = *(ControlCmd*)(pData + i); i += 2;
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
	*(ControlCmd*)pData = sCmd; pData += 2;
	memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
	*(WORD*)pData = sSum;
	return strOut.c_str();
}

bool CPacket::GetFilePath(std::string& strPath) {
	strPath = strData;
	return true;
}
bool CPacket::GetMouseEvent(MOUSEEV& mouse) {
	if (sCmd == ControlCmd::MouseEvent) {
		memcpy(&mouse, strData.c_str(), sizeof(MOUSEEV));
		return true;
	}
	return false;
}