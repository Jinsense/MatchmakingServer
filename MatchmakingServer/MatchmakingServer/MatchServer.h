#ifndef _MATCHINGSERVER_SERVER_MATCHING_H_
#define _MATCHINGSERVER_SERVER_MATCHING_H_

#include <list>
#include <map>

#include "Config.h"
#include "LanClient.h"
#include "NetServer.h"
#include "DBConnector.h"
#include "Player.h"

#include <cpprest\http_client.h>		
#include <cpprest\filestream.h>			

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

public:
	//-----------------------------------------------------------
	//	����� �Լ�
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen);
	bool	MatchDBSet();

	//-----------------------------------------------------------
	// OnClientJoin, OnClientLeave ���� ȣ��
	//-----------------------------------------------------------
	bool	InsertPlayer(unsigned __int64 iClientID);
	bool	RemovePlayer(unsigned __int64 iClientID);

	//-----------------------------------------------------------
	// OnRecv ���� �α������� ó�� �� ���,  UpdateThread ���� �����ð� ���� ������ ���.
	//-----------------------------------------------------------
	bool	DisconnectPlayer(unsigned __int64 iClientID, BYTE byStatus);

	//-----------------------------------------------------------
	// ���� ����� �� 
	//-----------------------------------------------------------
	int		GetPlayerCount(void);

	//-----------------------------------------------------------
	// ������ �÷��̾� ã�� 
	//-----------------------------------------------------------
	CPlayer*	FindPlayer_ClientID(unsigned __int64 iClientID);
//	PLAYER*	FindPlayer_AccountNo(INT64 AccountNo);

	//-----------------------------------------------------------
	// White IP ����
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

};

#endif _MATCHINGSERVER_SERVER_MATCHING_H_