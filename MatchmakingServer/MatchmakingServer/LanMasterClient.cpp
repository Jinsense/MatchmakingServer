#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <windows.h>

#include "MatchServer.h"
#include "LanMasterClient.h"

using namespace std;

CLanMasterClient::CLanMasterClient() :
	m_iRecvPacketTPS(NULL),
	m_iSendPacketTPS(NULL)
{
	m_Session = new LANSESSION;
	m_bReConnect = false;
	m_bRelease = false;

	setlocale(LC_ALL, "Korean");

	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
}

CLanMasterClient::~CLanMasterClient()
{
	delete m_Session;
}

void CLanMasterClient::Constructor(CMatchServer *pMatching)
{
	_pMatchingServer = pMatching;
	return;
}

void CLanMasterClient::OnEnterJoinServer()
{
	//	서버와의 연결 성공 후
	CPacket *pPacket = CPacket::Alloc();

	WORD Type = en_PACKET_MAT_MAS_REQ_SERVER_ON;
	int ServerNo = _pMatchingServer->_Config.SERVER_NO;
	char MasterToken[32] = { 0, };
	memcpy_s(&MasterToken, sizeof(MasterToken), &_pMatchingServer->_Config.MASTERTOKEN, sizeof(MasterToken));
	*pPacket << Type << ServerNo;
	pPacket->PushData((char*)&MasterToken, sizeof(MasterToken));

	SendPacket(pPacket);
	pPacket->Free();

	m_Session->bConnect = true;

	return;
}

void CLanMasterClient::OnLeaveServer()
{
	//	서버와의 연결이 끊어졌을 때
	m_Session->bConnect = false;
	m_bReConnect = true;
	m_bRelease = true;
	return;
}

void CLanMasterClient::OnLanRecv(CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	모니터링 측정 변수
	//-------------------------------------------------------------
	m_iRecvPacketTPS++;
	//-------------------------------------------------------------
	//	패킷 처리 - 컨텐츠 처리
	//-------------------------------------------------------------
	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	패킷 처리 - 매치메이킹 서버 켜짐 수신 확인
	//	Type : en_PACKET_MAT_MAS_RES_SERVER_ON
	//	int	 : ServerNo
	//-------------------------------------------------------------
	if (en_PACKET_MAT_MAS_RES_SERVER_ON == Type)
	{
		int ServerNo = NULL;
		*pPacket >> ServerNo;
		if (ServerNo != _pMatchingServer->_Config.SERVER_NO)
		{
			_pMatchingServer->_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"Master LanRecv ServerNo Not Same [My ServerNo : %d, Recv ServerNo : %d]"), _pMatchingServer->_Config.SERVER_NO, ServerNo);
			g_CrashDump->Crash();
			return;
		}
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 게임방 정보 응답
	//	Type : en_PACKET_MAT_MAS_RES_GAME_ROOM
	//	UINT64 : ClientKey
	//	BYTE : Status ( 1 : 성공 - 추가 정보 있음 / 0 : 실패 - 추가 정보 없음 )
	//-------------------------------------------------------------
	else if (en_PACKET_MAT_MAS_RES_GAME_ROOM == Type)
	{
		UINT64 ClientKey = NULL;
		unsigned __int64 ClientID = NULL;
		BYTE Status = NULL;
		*pPacket >> ClientKey >> Status;
		//	ClientKey로 ClientID를 구한다
		ClientID = _pMatchingServer->FindPlayer_ClientKey(ClientKey);

		Type = en_PACKET_CS_MATCH_RES_GAME_ROOM;
		CPacket * newPacket = CPacket::Alloc();
		*newPacket << Type << Status;
		if (0 == Status)
		{
			//	방 정보 얻기 실패
			_pMatchingServer->SendPacket(ClientID, newPacket);
			newPacket->Free();
			return;
		}
		else
		{
			//	방 정보 얻기 성공 - 추가적으로 패킷에서 추출
			WORD BattleServerNo = NULL;
			WCHAR IP[16] = { 0, };
			WORD Port = NULL;
			int RoomNo = NULL;
			char ConnectToken[32] = { 0, };
			char EnterToken[32] = { 0, };
			WCHAR ChatServerIP[16] = { 0, };
			WORD ChatServerPort = NULL;

			*pPacket >> BattleServerNo;
			pPacket->PopData((char*)&IP[0], sizeof(IP));
			*pPacket >> Port >> RoomNo;
			pPacket->PopData((char*)&ConnectToken, sizeof(ConnectToken));
			pPacket->PopData((char*)&EnterToken, sizeof(EnterToken));
			pPacket->PopData((char*)&ChatServerIP[0], sizeof(ChatServerIP));
			*pPacket >> ChatServerPort;

			*newPacket << BattleServerNo;
			newPacket->PushData((char*)&IP, sizeof(IP));
			*newPacket << Port << RoomNo;
			newPacket->PushData((char*)&ConnectToken, sizeof(ConnectToken));
			newPacket->PushData((char*)&EnterToken, sizeof(EnterToken));
			newPacket->PushData((char*)&ChatServerIP, sizeof(ChatServerIP));
			*newPacket << ChatServerPort;
			_pMatchingServer->SendPacket(ClientID, newPacket);
			newPacket->Free();
			return;
		}
	}

	return;
}

void CLanMasterClient::OnLanSend(int SendSize)
{
	//	패킷 송신 완료 후

	return;
}

void CLanMasterClient::OnWorkerThreadBegin()
{

}

void CLanMasterClient::OnWorkerThreadEnd()
{

}

void CLanMasterClient::OnError(int ErrorCode, WCHAR *pMsg)
{

}

bool CLanMasterClient::Connect(WCHAR * ServerIP, int Port, bool bNoDelay, int MaxWorkerThread)
{
	wprintf(L"[Client :: ClientInit]	Start\n");

	m_Session->RecvQ.Clear();
	m_Session->PacketQ.Clear();
	m_Session->SendFlag = false;
	m_bRelease = false;

	for (auto i = 0; i < MaxWorkerThread; i++)
	{
		m_hWorker_Thread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, (LPVOID)this, 0, NULL);
	}

	WSADATA wsaData;
	int retval = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != retval)
	{
		wprintf(L"[Client :: Connect]	WSAStartup Error\n");
		//	로그
		g_CrashDump->Crash();
		return false;
	}

	m_Session->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_Session->sock == INVALID_SOCKET)
	{
		wprintf(L"[Client :: Connect]	WSASocket Error\n");
		//	로그
		g_CrashDump->Crash();
		return false;
	}

	struct sockaddr_in client_addr;
	ZeroMemory(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	InetPton(AF_INET, ServerIP, &client_addr.sin_addr.s_addr);

	client_addr.sin_port = htons(Port);
	setsockopt(m_Session->sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));

	while (1)
	{
		retval = connect(m_Session->sock, (SOCKADDR*)&client_addr, sizeof(client_addr));
		if (retval == SOCKET_ERROR)
		{
			wprintf(L"[Client :: Connect]		Login_LanServer Not Connect\n");
			Sleep(1000);
			continue;
		}
		break;
	}
	InterlockedIncrement(&m_Session->IO_Count);
	CreateIoCompletionPort((HANDLE)m_Session->sock, m_hIOCP, (ULONG_PTR)this, 0);

	OnEnterJoinServer();
	wprintf(L"[Client :: Connect]		Complete\n");
	StartRecvPost();
	return true;
}

bool CLanMasterClient::Disconnect()
{
	closesocket(m_Session->sock);
	m_Session->sock = INVALID_SOCKET;

	while (0 < m_Session->SendQ.GetUseCount())
	{
		CPacket * pPacket;
		m_Session->SendQ.Dequeue(pPacket);
		if (pPacket == nullptr)
			g_CrashDump->Crash();
		pPacket->Free();
	}

	while (0 < m_Session->PacketQ.GetUseSize())
	{
		CPacket * pPacket;
		m_Session->PacketQ.Peek((char*)&pPacket, sizeof(CPacket*));
		if (pPacket == nullptr)
			g_CrashDump->Crash();
		pPacket->Free();
		m_Session->PacketQ.Dequeue(sizeof(CPacket*));
	}
	m_Session->bConnect = false;

	WaitForMultipleObjects(LANCLIENT_WORKERTHREAD, m_hWorker_Thread, false, INFINITE);

	for (auto iCnt = 0; iCnt < LANCLIENT_WORKERTHREAD; iCnt++)
	{
		CloseHandle(m_hWorker_Thread[iCnt]);
		m_hWorker_Thread[iCnt] = INVALID_HANDLE_VALUE;
	}

	WSACleanup();
	return true;
}

bool CLanMasterClient::IsConnect()
{
	return m_Session->bConnect;
}

bool CLanMasterClient::SendPacket(CPacket *pPacket)
{
	m_iSendPacketTPS++;
	pPacket->AddRef();
	pPacket->SetHeader_CustomShort(pPacket->GetDataSize());
	m_Session->SendQ.Enqueue(pPacket);
	SendPost();

	return true;
}

void CLanMasterClient::WorkerThread_Update()
{
	DWORD retval;

	while (!m_bRelease)
	{
		//	초기화 필수
		OVERLAPPED * pOver = NULL;
		LANSESSION * pSession = NULL;
		DWORD Trans = 0;

		retval = GetQueuedCompletionStatus(m_hIOCP, &Trans, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pOver, INFINITE);
		//		OnWorkerThreadBegin();

		if (nullptr == pOver)
		{
			if (FALSE == retval)
			{
				int LastError = GetLastError();
				if (WSA_OPERATION_ABORTED == LastError)
				{

				}
				//	IOCP 자체 오류
				g_CrashDump->Crash();
			}
			else if (0 == Trans)
			{
				PostQueuedCompletionStatus(m_hIOCP, 0, 0, 0);
			}
			else
			{
				//	현재 구조에서 발생할수 없는 상황
				g_CrashDump->Crash();
			}
			break;
		}

		if (0 == Trans)
		{
			shutdown(m_Session->sock, SD_BOTH);
			//			Disconnect();
		}
		else if (pOver == &m_Session->RecvOver)
		{
			CompleteRecv(Trans);
		}
		else if (pOver == &m_Session->SendOver)
		{
			CompleteSend(Trans);
		}

		if (0 >= (retval = InterlockedDecrement(&m_Session->IO_Count)))
		{
			if (0 == retval)
				Disconnect();
			else if (0 > retval)
				g_CrashDump->Crash();
		}
		//		OnWorkerThreadEnd();
	}

}

void CLanMasterClient::CompleteRecv(DWORD dwTransfered)
{
	m_Session->RecvQ.Enqueue(dwTransfered);
	WORD _wPayloadSize = 0;

	while (LANCLIENT_HEADERSIZE == m_Session->RecvQ.Peek((char*)&_wPayloadSize, LANCLIENT_HEADERSIZE))
	{
		if (m_Session->RecvQ.GetUseSize() < LANCLIENT_HEADERSIZE + _wPayloadSize)
			break;

		m_Session->RecvQ.Dequeue(LANCLIENT_HEADERSIZE);
		CPacket *_pPacket = CPacket::Alloc();
		if (_pPacket->GetFreeSize() < _wPayloadSize)
		{
			_pPacket->Free();
			Disconnect();
			return;
		}
		m_Session->RecvQ.Dequeue(_pPacket->GetWritePtr(), _wPayloadSize);
		_pPacket->PushData(_wPayloadSize + sizeof(CPacket::st_PACKET_HEADER));
		_pPacket->PopData(sizeof(CPacket::st_PACKET_HEADER));

		m_iRecvPacketTPS++;
		OnLanRecv(_pPacket);
		_pPacket->Free();
	}
	RecvPost();
}

void CLanMasterClient::CompleteSend(DWORD dwTransfered)
{
	CPacket * pPacket[LANCLIENT_WSABUFNUM];
	int Num = m_Session->Send_Count;

	m_Session->PacketQ.Peek((char*)&pPacket, sizeof(CPacket*) * Num);
	for (auto i = 0; i < Num; i++)
	{
		if (pPacket == nullptr)
			g_CrashDump->Crash();
		pPacket[i]->Free();
		m_Session->PacketQ.Dequeue(sizeof(CPacket*));
	}
	m_Session->Send_Count -= Num;

	InterlockedExchange(&m_Session->SendFlag, false);

	SendPost();
}

void CLanMasterClient::StartRecvPost()
{
	DWORD flags = 0;
	ZeroMemory(&m_Session->RecvOver, sizeof(m_Session->RecvOver));

	WSABUF wsaBuf[2];
	DWORD freeSize = m_Session->RecvQ.GetFreeSize();
	DWORD notBrokenPushSize = m_Session->RecvQ.GetNotBrokenPushSize();
	if (0 == freeSize && 0 == notBrokenPushSize)
	{
		//	로그
		//	RecvQ가 다 차서 서버에서 연결을 끊음
		g_CrashDump->Crash();
		return;
	}

	int numOfBuf = (notBrokenPushSize < freeSize) ? 2 : 1;

	wsaBuf[0].buf = m_Session->RecvQ.GetWriteBufferPtr();		//	Dequeue는 rear를 건드리지 않으므로 안전
	wsaBuf[0].len = notBrokenPushSize;

	if (2 == numOfBuf)
	{
		wsaBuf[1].buf = m_Session->RecvQ.GetBufferPtr();
		wsaBuf[1].len = freeSize - notBrokenPushSize;
	}

	if (SOCKET_ERROR == WSARecv(m_Session->sock, wsaBuf, numOfBuf, NULL, &flags, &m_Session->RecvOver, NULL))
	{
		int lastError = WSAGetLastError();
		if (ERROR_IO_PENDING != lastError)
		{
			if (0 != InterlockedDecrement(&m_Session->IO_Count))
			{
				_pMatchingServer->_pLog->Log(L"Error", LOG_SYSTEM, L"Recv SocketError - Code %d", lastError);
				shutdown(m_Session->sock, SD_BOTH);
			}
			//			Disconnect();
		}
	}
	return;
}

void CLanMasterClient::RecvPost()
{
	int Count = InterlockedIncrement(&m_Session->IO_Count);
	if (1 == Count)
	{
		InterlockedDecrement(&m_Session->IO_Count);
		return;
	}

	DWORD flags = 0;
	ZeroMemory(&m_Session->RecvOver, sizeof(m_Session->RecvOver));

	WSABUF wsaBuf[2];
	DWORD freeSize = m_Session->RecvQ.GetFreeSize();
	DWORD notBrokenPushSize = m_Session->RecvQ.GetNotBrokenPushSize();
	if (0 == freeSize && 0 == notBrokenPushSize)
	{
		//	로그
		//	RecvQ가 다 차서 서버에서 연결을 끊음
		g_CrashDump->Crash();
		return;
	}

	int numOfBuf = (notBrokenPushSize < freeSize) ? 2 : 1;

	wsaBuf[0].buf = m_Session->RecvQ.GetWriteBufferPtr();		//	Dequeue는 rear를 건드리지 않으므로 안전
	wsaBuf[0].len = notBrokenPushSize;

	if (2 == numOfBuf)
	{
		wsaBuf[1].buf = m_Session->RecvQ.GetBufferPtr();
		wsaBuf[1].len = freeSize - notBrokenPushSize;
	}

	if (SOCKET_ERROR == WSARecv(m_Session->sock, wsaBuf, numOfBuf, NULL, &flags, &m_Session->RecvOver, NULL))
	{
		int lastError = WSAGetLastError();
		if (ERROR_IO_PENDING != lastError)
		{
			if (0 != InterlockedDecrement(&m_Session->IO_Count))
			{
				_pMatchingServer->_pLog->Log(L"shutdown", LOG_SYSTEM, L"LanClient Recv SocketError - Code %d", lastError);
				shutdown(m_Session->sock, SD_BOTH);
			}
			//			Disconnect();
		}
	}
	return;
}

void CLanMasterClient::SendPost()
{
	do
	{
		if (true == InterlockedCompareExchange(&m_Session->SendFlag, true, false))
			return;

		if (0 == m_Session->SendQ.GetUseCount())
		{
			InterlockedExchange(&m_Session->SendFlag, false);
			return;
		}

		WSABUF wsaBuf[LANCLIENT_WSABUFNUM];
		CPacket *pPacket;
		long BufNum = 0;
		int iUseSize = (m_Session->SendQ.GetUseCount());
		if (iUseSize > LANCLIENT_WSABUFNUM)
		{
			//	SendQ에 패킷이 100개 이상있을때, WSABUF에 100개만 넣는다.
			BufNum = LANCLIENT_WSABUFNUM;
			m_Session->Send_Count = LANCLIENT_WSABUFNUM;
			//	pSession->SendQ.Peek((char*)&pPacket, sizeof(CPacket*) * MAX_WSABUF_NUMBER);
			for (auto i = 0; i < LANCLIENT_WSABUFNUM; i++)
			{
				m_Session->SendQ.Dequeue(pPacket);
				m_Session->PacketQ.Enqueue((char*)&pPacket, sizeof(CPacket*));
				wsaBuf[i].buf = pPacket->GetReadPtr();
				wsaBuf[i].len = pPacket->GetPacketSize_CustomHeader(LANCLIENT_HEADERSIZE);
			}
		}
		else
		{
			//	SendQ에 패킷이 100개 이하일 때 현재 패킷을 계산해서 넣는다
			BufNum = iUseSize;
			m_Session->Send_Count = iUseSize;
			//			pSession->SendQ.Peek((char*)&pPacket, sizeof(CPacket*) * iUseSize);
			for (auto i = 0; i < iUseSize; i++)
			{
				m_Session->SendQ.Dequeue(pPacket);
				m_Session->PacketQ.Enqueue((char*)&pPacket, sizeof(CPacket*));
				wsaBuf[i].buf = pPacket->GetReadPtr();
				wsaBuf[i].len = pPacket->GetPacketSize_CustomHeader(LANCLIENT_HEADERSIZE);
			}
		}

		if (1 == InterlockedIncrement(&m_Session->IO_Count))
		{
			InterlockedDecrement(&m_Session->IO_Count);
			InterlockedExchange(&m_Session->SendFlag, false);
			return;
		}

		ZeroMemory(&m_Session->SendOver, sizeof(m_Session->SendOver));
		if (SOCKET_ERROR == WSASend(m_Session->sock, wsaBuf, BufNum, NULL, 0, &m_Session->SendOver, NULL))
		{
			int lastError = WSAGetLastError();
			if (ERROR_IO_PENDING != lastError)
			{
				if (0 != InterlockedDecrement(&m_Session->IO_Count))
				{
					_pMatchingServer->_pLog->Log(L"shutdown", LOG_SYSTEM, L"LanClient Send SocketError - Code %d", lastError);
					shutdown(m_Session->sock, SD_BOTH);
				}
				//				Disconnect();
				return;
			}
		}
	} while (0 != m_Session->SendQ.GetUseCount());
}