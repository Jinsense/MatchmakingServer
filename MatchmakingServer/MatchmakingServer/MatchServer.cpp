#include "MatchServer.h"

CMatchServer::CMatchServer()
{

}

CMatchServer::~CMatchServer()
{

}

void CMatchServer::OnClientJoin(st_SessionInfo Info)
{
	//	�ʿ� ���� �߰�

	return;
}

void CMatchServer::OnClientLeave(unsigned __int64 ClientID)
{
	//	�ʿ� ���� ����

	return;
}

void CMatchServer::OnConnectionRequest(WCHAR * pClientID)
{

	return;
}

void CMatchServer::OnError(int ErrorCode, WCHAR *pError)
{

	return;
}

bool CMatchServer::OnRecv(unsigned __int64 ClientID, CPacket *pPacket)
{
	//	��Ŷ ó�� - ������ ó��
	
	return true;
}

void CMatchServer::ConfigSet()
{

	return;
}

void CMatchServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CMatchServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}

bool CMatchServer::InsertPlayer(unsigned __int64 iClientID)
{

	return true;
}

bool CMatchServer::RemovePlayer(unsigned __int64 iClientID)
{

	return true;
}

bool CMatchServer::DisconnectPlayer(unsigned __int64 iClientID, BYTE byStatus)
{

	return true;
}

int CMatchServer::GetPlayerCount()
{
	return _PlayerMap.size();
}

void CMatchServer::MonitorThread_Update()
{

	return;
}