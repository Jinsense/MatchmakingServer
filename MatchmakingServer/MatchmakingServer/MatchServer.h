#ifndef _MATCHINGSERVER_SERVER_MATCHING_H_
#define _MATCHINGSERVER_SERVER_MATCHING_H_

#include <list>
#include <map>
#include <stack>
#include <windows.h>
#include <winhttp.h>
#include <chrono>

#include <cpprest\http_client.h>
#include <cpprest\filestream.h>	

#include "Config.h"
#include "CpuUsage.h"
#include "EtherNet_PDH.h"
#include "LanClient.h"
#include "NetServer.h"
#include "DBConnector.h"
#include "Player.h"	
#include "ShDB_ErrorCode.h"

#pragma comment (lib, "winhttp.lib")
//	#pragma comment(lib, "cpprest120_2_4")		// Windows Only
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace std;

#define		SET_CLIENTKEY(ServerNo, ClientID)		ServerNo = ServerNo << 24; ClientID = ServerNo | ClientID;


enum en_RES_LOGIN
{
	LOGIN_SUCCESS = 1,
	SESSIONKEY_ERROR = 2,
	ACCOUNTNO_NOT_EXIST = 3,
	ETC_ERROR = 4,
	VER_ERROR = 5,
};

class CLanClient;

class CMatchServer : public CNetServer
{
public:
	CMatchServer();
	virtual ~CMatchServer();

protected:
	//-----------------------------------------------------------
	// �����Լ���
	//-----------------------------------------------------------
	virtual void	OnClientJoin(st_SessionInfo Info);
	virtual void	OnClientLeave(unsigned __int64 ClientID);
	virtual void	OnConnectionRequest(WCHAR * pClientID);
	virtual void	OnError(int ErrorCode, WCHAR *pError);
	virtual bool	OnRecv(unsigned __int64 ClientID, CPacket *pPacket);

	//-----------------------------------------------------------
	// ��Ī���� �����췯 ����
	//-----------------------------------------------------------
	//	���� ���� ����� Ÿ�Ӿƿ� �Լ�
	//	��Ī status DB�� Ÿ�ӽ����� ���� ���� ������ �Լ�
	static unsigned int WINAPI HeartbeatThread(LPVOID arg)
	{
		CMatchServer *_pHeartbeatThread = (CMatchServer *)arg;
		if (NULL == _pHeartbeatThread)
		{
			std::wprintf(L"[Server :: HeartbeatThread] Init Error\n");
			return false;
		}
		_pHeartbeatThread->HeartbeatThread_Update();
		return true;
	}
	//-----------------------------------------------------------
	//	����͸� ����
	//-----------------------------------------------------------
	static unsigned int WINAPI LanMonitoringThread(LPVOID arg)
	{
		CMatchServer *_pLanMonitoringThread = (CMatchServer *)arg;
		if (NULL == _pLanMonitoringThread)
		{
			std::wprintf(L"[Server :: LanMonitoringThread] Init Error\n");
			return false;
		}
		_pLanMonitoringThread->LanMonitoringThread_Update();
		return true;
	}
	//-----------------------------------------------------------
	//	������ ���� ���� üũ
	//-----------------------------------------------------------
	static unsigned int WINAPI LanMasterCheckThread(LPVOID arg)
	{
		CMatchServer *_pLanMasterCheckThread = (CMatchServer *)arg;
		if (NULL == _pLanMasterCheckThread)
		{
			std::wprintf(L"[Server :: LanMonitoringThread] Init Error\n");
			return false;
		}
		_pLanMasterCheckThread->LanMasterCheckThead_Update();
		return true;
	}

	virtual void	MonitorThread_Update();
	void	HeartbeatThread_Update();
	void	LanMonitoringThread_Update();
	void	LanMasterCheckThead_Update();

public:
	//-----------------------------------------------------------
	//	����� �Լ�
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int BufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int BufLen);
	bool	MatchDBSet();
	bool	LanMonitorSendPacket(BYTE DataType);

	//-----------------------------------------------------------
	// OnClientJoin, OnClientLeave ���� ȣ��
	//-----------------------------------------------------------
	bool	InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer);
	bool	RemovePlayer(unsigned __int64 ClientID);

	//-----------------------------------------------------------
	// TimeOut ���� ����
	//-----------------------------------------------------------
	bool	DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo);

	//-----------------------------------------------------------
	// ���� ����� �� 
	//-----------------------------------------------------------
	int		GetPlayerCount(void);

	//-----------------------------------------------------------
	// ������ �÷��̾� ã�� 
	//-----------------------------------------------------------
	CPlayer*	FindPlayer_ClientID(unsigned __int64 ClientID);
//	PLAYER*	FindPlayer_AccountNo(INT64 AccountNo);

	//-----------------------------------------------------------
	// White IP ����
	//-----------------------------------------------------------


public:
	CLanClient *	_pMaster;
	CLanClient *	_pMonitor;
	CDBConnector	_StatusDB;
	CSystemLog *	_pLog;

protected:
	SRWLOCK		_DB_srwlock;
	HANDLE		_hHeartbeatThread;

	//-------------------------------------------------------------
	// ������ ����.
	// 
	// �����ڴ� �� ����Ʈ�� ������.  ������ ����ȭ�� ���� SRWLock �� ����Ѵ�.
	//-------------------------------------------------------------
	std::map<unsigned __int64, CPlayer*>	_PlayerMap;
	SRWLOCK				_PlayerMap_srwlock;

	CMemoryPool<CPlayer> *_PlayerPool;
	//-------------------------------------------------------------
	// ����͸��� ����
	//-------------------------------------------------------------
	long	_JoinSession;				//	�α��� ���� ���� ��
	long	_EnterRoomTPS;				//	�� ���� ���� ��
	int	_TimeStamp;						//	TimeStamp
	int	_CPU_Total;						//	CPU ��ü �����
	int	_Available_Memory;				//	��밡���� �޸�
	int	_Network_Recv;					//	�ϵ���� �̴��� ����
	int	_Network_Send;					//	�ϵ���� �̴��� �۽�
	int	_Nonpaged_Memory;				//	���������� �޸�
	int	_MatchServer_On;				//	��ġ����ŷ ���� ON
	int	_MatchServer_CPU;				//	��ġ����ŷ CPU ���� (Ŀ�� + ����)
	int	_MatchServer_Memory_Commit;		//	��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
	int	_MatchServer_PacketPool;		//	��ġ����ŷ ��ŶǮ ��뷮
	int	_MatchServer_Session;			//	��ġ����ŷ ���� ����
	int	_MatchServer_Player;			//	��ġ����ŷ ���� ���� (�α��� ���� ��)
	int _MatchServer_MatchSuccess;		//	��ġ����ŷ �� ���� ���� �� (�ʴ�)

	CCpuUsage	_Cpu;
	CEthernet	_Ethernet;
	HANDLE		_hLanMonitorThread;
	HANDLE		_hLanMasterThread;

	PDH_HQUERY		_CpuQuery;
	PDH_HCOUNTER	_MemoryAvailableMBytes;
	PDH_HCOUNTER	_MemoryNonpagedBytes;
	PDH_HCOUNTER	_ProcessPrivateBytes;
	PDH_FMT_COUNTERVALUE _CounterVal;
};

#endif _MATCHINGSERVER_SERVER_MATCHING_H_