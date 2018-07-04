#ifndef _MATCHINGSERVER_LIB_LANCLIENT_H_
#define _MATCHINGSERVER_LIB_LANCLIENT_H_


#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include "Packet.h"
#include "RingBuffer.h"
#include "LockFreeQueue.h"
#include "CommonProtocol.h"

#define LANCLIENT_WORKERTHREAD	2
#define LANCLIENT_WSABUFNUM		100
#define LANCLIENT_QUEUESIZE		20000

#define LANCLIENT_HEADERSIZE	2

typedef struct st_Client
{
	bool		bConnect;
	long		SendFlag;
	long		Send_Count;
	long		IO_Count;
	OVERLAPPED				SendOver, RecvOver;
	CRingBuffer				RecvQ, PacketQ;
	CLockFreeQueue<CPacket*> SendQ;
	SOCKET					sock;

	st_Client() :
		RecvQ(LANCLIENT_QUEUESIZE),
		PacketQ(LANCLIENT_QUEUESIZE),
		SendFlag(false),
		bConnect(false),
		IO_Count(0) {}
}LANSESSION;

class CMatchServer;

class CLanClient
{
public:
	CLanClient();
	~CLanClient();

	bool Connect(WCHAR * ServerIP, int Port, bool NoDelay, int MaxWorkerThread);	//   ���ε� IP, ����IP / ��Ŀ������ �� / ���ۿɼ�
	bool Disconnect();
	bool IsConnect();
	bool SendPacket(CPacket *pPacket);

	void Constructor(CMatchServer *pMatch);

	void OnEnterJoinServer();		//	�������� ���� ���� ��
	void OnLeaveServer();			//	�������� ������ �������� ��

	void OnLanRecv(CPacket *pPacket);	//	�ϳ��� ��Ŷ ���� �Ϸ� ��
	void OnLanSend(int SendSize);		//	��Ŷ �۽� �Ϸ� ��

	void OnWorkerThreadBegin();
	void OnWorkerThreadEnd();

	void OnError(int ErrorCode, WCHAR *pMsg);

private:
	static unsigned int WINAPI WorkerThread(LPVOID arg)
	{
		CLanClient * pWorkerThread = (CLanClient *)arg;
		if (pWorkerThread == NULL)
		{
			wprintf(L"[Client :: WorkerThread]	Init Error\n");
			return FALSE;
		}

		pWorkerThread->WorkerThread_Update();
		return TRUE;
	}
	void WorkerThread_Update();
	void StartRecvPost();
	void RecvPost();
	void SendPost();
	void CompleteRecv(DWORD dwTransfered);
	void CompleteSend(DWORD dwTransfered);

public:
	long		m_iRecvPacketTPS;
	long		m_iSendPacketTPS;
	LANSESSION	*m_Session;

private:
	HANDLE					m_hIOCP;
	HANDLE					m_hWorker_Thread[LANCLIENT_WORKERTHREAD];

	CMatchServer *			_pMatchingServer;

};

#endif
