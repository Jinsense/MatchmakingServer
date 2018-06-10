#ifndef _MATCHINGSERVER_SERVER_MATCHING_H_
#define _MATCHINGSERVER_SERVER_MATCHING_H_

#include <list>
#include <map>

#include "Config.h"
#include "LanClient.h"
#include "NetServer.h"
#include "DBConnector.h"
#include "Player.h"


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

public:
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen);

	/////////////////////////////////////////////////////////////
	// OnClientJoin, OnClientLeave 에서 호출
	/////////////////////////////////////////////////////////////
	bool	InsertPlayer(unsigned __int64 iClientID);
	bool	RemovePlayer(unsigned __int64 iClientID);

	/////////////////////////////////////////////////////////////
	// OnRecv 에서 로그인인증 처리 후 사용,  UpdateThread 에서 일정시간 지난 유저에 사용.
	/////////////////////////////////////////////////////////////
	bool	DisconnectPlayer(unsigned __int64 iClientID, BYTE byStatus);

	/////////////////////////////////////////////////////////////
	// 접속 사용자 수 
	/////////////////////////////////////////////////////////////
	int		GetPlayerCount(void);

	/////////////////////////////////////////////////////////////
	// 접속한 플레이어 찾기 
	/////////////////////////////////////////////////////////////
//	PLAYER*	FindPlayer_ClientID(unsigned __int64 iClientID);
//	PLAYER*	FindPlayer_AccountNo(INT64 AccountNo);

	/////////////////////////////////////////////////////////////
	// White IP 관련
	/////////////////////////////////////////////////////////////

private:
	virtual void	MonitorThread_Update();

public:
	CLanClient *	_pMaster;
	CLanClient *	_pMonitor;
	CDBConnector	_StatusDB;
	CSystemLog *	_pLog;
	CConfig			_Config;

protected:
	SRWLOCK		_DB_srwlock;

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

};

#endif _MATCHINGSERVER_SERVER_MATCHING_H_