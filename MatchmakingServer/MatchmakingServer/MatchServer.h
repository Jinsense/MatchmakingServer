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
	// 가상함수들
	//-----------------------------------------------------------
	virtual void	OnClientJoin(st_SessionInfo Info);
	virtual void	OnClientLeave(unsigned __int64 ClientID);
	virtual void	OnConnectionRequest(WCHAR * pClientID);
	virtual void	OnError(int ErrorCode, WCHAR *pError);
	virtual bool	OnRecv(unsigned __int64 ClientID, CPacket *pPacket);

	//-----------------------------------------------------------
	// 매칭서버 스케쥴러 역할
	//-----------------------------------------------------------
	//	접속 중인 사용자 타임아웃 함수
	//	매칭 status DB에 타임스탬프 갱신 쿼리 보내는 함수
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
	//	모니터링 전송
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
	//	마스터 서버 연결 체크
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
	//	사용자 함수
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int BufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int BufLen);
	bool	MatchDBSet();
	bool	LanMonitorSendPacket(BYTE DataType);

	//-----------------------------------------------------------
	// OnClientJoin, OnClientLeave 에서 호출
	//-----------------------------------------------------------
	bool	InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer);
	bool	RemovePlayer(unsigned __int64 ClientID);

	//-----------------------------------------------------------
	// TimeOut 유저 끊기
	//-----------------------------------------------------------
	bool	DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo);

	//-----------------------------------------------------------
	// 접속 사용자 수 
	//-----------------------------------------------------------
	int		GetPlayerCount(void);

	//-----------------------------------------------------------
	// 접속한 플레이어 찾기 
	//-----------------------------------------------------------
	CPlayer*	FindPlayer_ClientID(unsigned __int64 ClientID);
//	PLAYER*	FindPlayer_AccountNo(INT64 AccountNo);

	//-----------------------------------------------------------
	// White IP 관련
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
	// 접속자 관리.
	// 
	// 접속자는 본 리스트로 관리함.  스레드 동기화를 위해 SRWLock 을 사용한다.
	//-------------------------------------------------------------
	std::map<unsigned __int64, CPlayer*>	_PlayerMap;
	SRWLOCK				_PlayerMap_srwlock;

	CMemoryPool<CPlayer> *_PlayerPool;
	//-------------------------------------------------------------
	// 모니터링용 변수
	//-------------------------------------------------------------
	long	_JoinSession;				//	로그인 성공 유저 수
	long	_EnterRoomTPS;				//	방 배정 성공 수
	int	_TimeStamp;						//	TimeStamp
	int	_CPU_Total;						//	CPU 전체 사용율
	int	_Available_Memory;				//	사용가능한 메모리
	int	_Network_Recv;					//	하드웨어 이더넷 수신
	int	_Network_Send;					//	하드웨어 이더넷 송신
	int	_Nonpaged_Memory;				//	논페이지드 메모리
	int	_MatchServer_On;				//	매치메이킹 서버 ON
	int	_MatchServer_CPU;				//	매치메이킹 CPU 사용률 (커널 + 유저)
	int	_MatchServer_Memory_Commit;		//	매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
	int	_MatchServer_PacketPool;		//	매치메이킹 패킷풀 사용량
	int	_MatchServer_Session;			//	매치메이킹 접속 세션
	int	_MatchServer_Player;			//	매치메이킹 접속 유저 (로그인 성공 후)
	int _MatchServer_MatchSuccess;		//	매치메이킹 방 배정 성공 수 (초당)

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