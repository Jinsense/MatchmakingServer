#ifndef _MATCHINGSERVER_SERVER_MATCHING_H_
#define _MATCHINGSERVER_SERVER_MATCHING_H_

#include <list>
#include <map>
#include <WinInet.h>

#include <cpprest\http_client.h>		
#include <cpprest\filestream.h>	

#include "Config.h"
#include "LanClient.h"
#include "NetServer.h"
#include "DBConnector.h"
#include "Player.h"	
#include "ShDB_ErrorCode.h"

#include "../json/include/rapidjson/document.h"
#include "../json/include/rapidjson/writer.h"
#include "../json/include/rapidjson/stringbuffer.h"

#pragma comment(lib, "cpprest120_2_4")	// Windows Only
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace rapidjson;
using namespace std;

#define		SET_CLIENTKEY(ServerNo, ClientID)		ServerNo = ServerNo << 48; ClientID = ServerNo | ClientID;


StringBuffer StringJSON;
Writer<StringBuffer, UTF16<>> writer(StringJSON);

enum en_RES_LOGIN
{
	SUCCESS = 1,
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

public:
	//-----------------------------------------------------------
	//	사용자 함수
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int BufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int BufLen);
	bool	MatchDBSet();

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