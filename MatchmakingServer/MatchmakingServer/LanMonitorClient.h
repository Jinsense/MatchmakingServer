#ifndef _MATCHINGSERVER_SERVER_LANMONITORCLIENT_H_
#define _MATCHINGSERVER_SERVER_LANMONITORCLIENT_H_


#include "LanClient.h"

class CMatchServer;

class CLanMonitorClient
{
public:
	CLanMonitorClient();
	~CLanMonitorClient();

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
		CLanMonitorClient * pWorkerThread = (CLanMonitorClient *)arg;
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
	bool		m_bRelease;
	HANDLE		m_hIOCP;
	HANDLE		m_hWorker_Thread[LANCLIENT_WORKERTHREAD];

	CMatchServer *	_pMatchingServer;

};

#endif
