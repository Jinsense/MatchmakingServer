#include "MatchServer.h"

CMatchServer::CMatchServer()
{
	InitializeSRWLock(&_DB_srwlock);
	InitializeSRWLock(&_PlayerMap_srwlock);
	_PlayerPool = new CMemoryPool<CPlayer>();
	_pMaster = new CLanClient;
	_pMaster->Constructor(this);
	_pMonitor = new CLanClient;
	_pMonitor->Constructor(this);
	_pLog->GetInstance();

}

CMatchServer::~CMatchServer()
{
	delete _PlayerPool;
	delete _pMaster;
	delete _pMonitor;
}

void CMatchServer::OnClientJoin(st_SessionInfo Info)
{
	//-------------------------------------------------------------
	//	맵에 유저 추가
	//-------------------------------------------------------------
	CPlayer *pPlayer = _PlayerPool->Alloc();
	pPlayer->_ClientID = Info.iClientID;
	unsigned __int64 temp = pPlayer->_ClientID;
	strcpy_s(pPlayer->_ClientIP, sizeof(pPlayer->_ClientIP), Info.IP);
	pPlayer->_ClientKey = SET_CLIENTKEY(_Config.SERVER_NO, temp);
	pPlayer->_Time = GetTickCount64();
	InsertPlayer(pPlayer->_ClientID, pPlayer);
	return;
}

void CMatchServer::OnClientLeave(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	맵에 유저 삭제
	//-------------------------------------------------------------
	RemovePlayer(ClientID);
	return;
}

void CMatchServer::OnConnectionRequest(WCHAR * pClientID)
{
	//-------------------------------------------------------------
	//	화이트 IP 처리
	//	현재 사용 X
	//-------------------------------------------------------------
	return;
}

void CMatchServer::OnError(int ErrorCode, WCHAR *pError)
{
	//-------------------------------------------------------------
	//	에러 처리 - 로그 남김
	//	별도의 합의된 로그 코드를 통해 로그를 txt에 저장
	//	현재 사용 X
	//-------------------------------------------------------------
	return;
}

bool CMatchServer::OnRecv(unsigned __int64 ClientID, CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	모니터링 측정 변수
	//-------------------------------------------------------------
	m_iRecvPacketTPS++;
	//-------------------------------------------------------------
	//	예외 처리 - 현재 존재하는 유저인지 검사
	//-------------------------------------------------------------
	CPlayer *pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		Disconnect(ClientID);
		return false;
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 컨텐츠 처리
	//-------------------------------------------------------------
	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	패킷 처리 - 매치메이킹 서버로 로그인 요청
	//	Type	: en_PACKET_CS_MATCH_REQ_LOGIN
	//	INT64	: AccountNo
	//	char	: SessionKey[64]
	//	UINT	: Ver_Code
	//	
	//	응답	: en_PACKET_CS_MATCH_RES_LOGIN
	//	WORD	: Type
	//	BYTE	: Status
	//-------------------------------------------------------------
	if (en_PACKET_CS_MATCH_REQ_LOGIN == Type)
	{
		UINT Ver_Code;
		*pPacket >> pPlayer->_AccountNo;
		pPacket->PopData((char*)&pPlayer->_SessionKey, sizeof(pPlayer->_SessionKey));
		*pPacket >> Ver_Code;

		//	Config의 버전 코드와 맞는지 비교를 한다.
		if (Ver_Code != _Config.VER_CODE)
		{
			//	버전 코드가 다를 경우 로그를 남기고 버전오류 응답을 보낸 후 끊는다.
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
				const_cast<WCHAR*>(L"Ver_Code Not Same [AccountNo : %d]"), pPlayer->_AccountNo);
			CPacket * newPacket = CPacket::Alloc();
			Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = VER_ERROR;
			*newPacket << Type << Status;
			SendPacketAndDisConnect(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}

		//	JSON 데이터 - AccountNo를 생성한 후 API서버의 select_account.php 요청
		//	JSON 응답을 파싱한 후 세션키를 비교 한다.
		json::value PostData;
		json::value ResData;
		PostData[L"accountno"] = json::value::number(pPlayer->_AccountNo);
//		http_client Client(L"http://172.16.2.2:11701/select_account.php");
		http_client Client(_Config.APISERVER_IP);
		Client.request(methods::POST, L"", PostData.as_string().c_str(),
			L"application/json").then([&ResData](http_response response)
		{
			if (response.status_code() == status_codes::OK)
			{
				json::value Temp = response.body();
				ResData = Temp;
			}
		});
		//	result 값 확인
		if (SUCCESS != ResData[L"result"].as_integer())
		{
			if (NOT_JOIN == ResData[L"result"].as_integer())
				BYTE Status = ACCOUNTNO_NOT_EXIST;
			else
				BYTE Status = ETC_ERROR;
			CPacket * newPacket = CPacket::Alloc();
			Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = SESSIONKEY_ERROR;
			*newPacket << Type << Status;
			SendPacketAndDisConnect(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}
//		if (0 != strcmp(ResData[L"sessionkey"].as_string(), pPlayer->_SessionKey))
//		{

//		}
		char sessionkey[64] = { 0, };
		memcpy_s(&sessionkey, sizeof(ResData[L"sessionkey"]), &ResData[L"sessionkey"], sizeof(ResData[L"sessionkey"]));
		if (0 != strcmp(sessionkey, pPlayer->_SessionKey))
		{
			//	세션키가 다름 응답 후 끊기
			CPacket * newPacket = CPacket::Alloc();
			Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = SESSIONKEY_ERROR;
			*newPacket << Type << Status;
			SendPacketAndDisConnect(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}
		
		//	전부 맞다면 로그인 응답 패킷을 보내준다.
		CPacket * newPacket = CPacket::Alloc();
		Type = en_PACKET_CS_MATCH_RES_LOGIN;
		BYTE Status = LOGIN_SUCCESS;
		*newPacket << Type << Status;
		SendPacket(pPlayer->_ClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 방 정보 요청
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM
	//	
	//	응답	: 마스터 서버에게 ClientKey 와 방 정보 요청을 보냄
	//			  마스터 서버에게 정보를 받은 후 돌려 줌 
	//-------------------------------------------------------------
	else if (en_PACKET_CS_MATCH_REQ_GAME_ROOM == Type)
	{
		//	마스터 서버에 방 정보 요청을 보냄
		CPacket *newPacket = CPacket::Alloc();
		Type = en_PACKET_MAT_MAS_REQ_GAME_ROOM;
		*newPacket << Type << pPlayer->_ClientKey;
		_pMaster->SendPacket(newPacket);
		return true;
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 방 입장 성공 알림
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER
	//	WORD	: BattleServerNo
	//	int		: RoomNo
	//
	//	응답	: 클라이언트가 배틀서버 방 입장을 성공 함
	//			  마스터 서버에게 방 입장 성공을 전달 
	//-------------------------------------------------------------
	else if (en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER == Type)
	{
		WORD BattleServerNo;
		int RoomNo;
		*pPacket >> BattleServerNo >> RoomNo;
		//	마스터 서버에 방 정보 성공을 보냄
		CPacket *newPacket = CPacket::Alloc();
		Type = en_PACKET_MAT_MAS_REQ_ROOM_ENTER_SUCCESS;
		*newPacket << Type << BattleServerNo << RoomNo << pPlayer->_ClientKey;
		_pMaster->SendPacket(newPacket);
		return true;
	}
	
	return true;
}

void CMatchServer::HeartbeatThread_Update()
{
	//-------------------------------------------------------------
	//	Heartbeat Thread
	//	StatusDB에 일정시간마다 현재 매치서버 Timestamp 값 갱신
	//	전체 유저 대상으로 Heartbeat 기준으로 TimeOut 체크 
	//	현재 유저가 범위 이상 변화가 발생했을 때 DB에 connectuser 값 갱신
	//-------------------------------------------------------------
	UINT64 start = GetTickCount64();
	int count = GetPlayerCount();
	int usercount = NULL;
	while (1)
	{
		Sleep(1000);
		UINT64 now = GetTickCount64();
		if (now - start > _Config.DB_TIME_UPDATE)
		{
			if (false == _StatusDB.Query_Save(L"update `server` set `heartbeat` = now() where serverno = %d", _Config.SERVER_NO))
				_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB HEARTBEAT UPDATE FAIL [ServerNo : %d]"), _Config.SERVER_NO);
			else
				start = GetTickCount64();
		}
		AcquireSRWLockExclusive(&_PlayerMap_srwlock);
		for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
		{
			if (now - i->second->_Time > _Config.USER_TIMEOUT)
			{
				_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"USER TIMEOUT : %d [AccountNo : %d]"), now - i->second->_Time, i->second->_AccountNo);
				Disconnect(i->second->_ClientID);
			}
		}
		ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
		usercount = GetPlayerCount();
		if (_Config.USER_CHANGE < abs(count - usercount))
		{
			if (false == _StatusDB.Query_Save(L"update `server` set `connectuser` = %d where serverno = %d", usercount, _Config.SERVER_NO))
			{
				_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB CONNECTUSER UPDATE FAIL [ServerNo : %d]"), _Config.SERVER_NO);
			}
		}
	}
}

void CMatchServer::ConfigSet()
{
	//-------------------------------------------------------------
	//	Config 파일 불러오기
	//-------------------------------------------------------------
	_Config.Set();
	return;
}

void CMatchServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF8 타입을 UTF16으로 변환
	//-------------------------------------------------------------
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CMatchServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF16 타입을 UTF8로 변환
	//-------------------------------------------------------------
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}

bool CMatchServer::MatchDBSet()
{
	//-------------------------------------------------------------
	//	Matchmaking Status DB에 매칭서버 정보 insert / update
	//-------------------------------------------------------------
	//	현재 자신의 IP를 구한다.
	IN_ADDR myip = GetMyIPAddress();

	//	기존 서버번호가 있는 경우 INSERT 에러 발생
	bool bRes = _StatusDB.Query_Save(L"INSERT INTO `server` (`serverno`, `ip`, `port`, `connectuser`, `heartbeat` ) VALUES (%d, %s, $d, NOW())", _Config.SERVER_NO, myip, _Config.BIND_PORT, GetPlayerCount());
	if (false == bRes)
	{
		bRes = _StatusDB.Query_Save(L"UPDATE `server` set `ip` = %s, `port` = %d, `connectuser` = %d, `heartbeat` = NOW() where `serverno` = %d", myip, _Config.BIND_PORT, GetPlayerCount(), _Config.SERVER_NO);
		if (false == bRes)
		{
			//	Status DB에 매치서버 등록 실패
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB INSERT/UPDATE FAIL"));
			g_CrashDump->Crash();
		}
	}

	//	하트비트 스레드 생성
	_HeartbeatThread = (HANDLE)_beginthreadex(NULL, 0, &HeartbeatThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	HeartbeatThread Create\n");
	return true;
}

bool CMatchServer::InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 추가
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.insert(make_pair(ClientID, pPlayer));
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CMatchServer::RemovePlayer(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 제거
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.erase(ClientID);
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CMatchServer::DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo)
{
	//-------------------------------------------------------------
	//	TimeOut 유저 끊기
	//	Log 남기고 Disconnect 호출
	//-------------------------------------------------------------
	_pLog->Log(const_cast<WCHAR*>(L"TimeOut"), LOG_SYSTEM,
		const_cast<WCHAR*>(L"TimeOut [AccountNo : %d]"), AccountNo);
	Disconnect(ClientID);
	return true;
}

int CMatchServer::GetPlayerCount()
{
	//-------------------------------------------------------------
	//	현재 접속 중인 플레이어 수 얻기
	//-------------------------------------------------------------
	return _PlayerMap.size();
}

CPlayer* CMatchServer::FindPlayer_ClientID(unsigned __int64 ClientID)
{
	CPlayer *pPlayer = nullptr;
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	pPlayer = _PlayerMap.find(ClientID)->second;
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return pPlayer;
}

void CMatchServer::MonitorThread_Update()
{
	//-------------------------------------------------------------
	//	모니터링 스레드 - NetServer에서 가상함수 상속받아 사용
	//	모니터링 할 항목을 1초 단위로 갱신하여 콘솔 화면에 출력
	//
	//	모니터링 항목들
	//-------------------------------------------------------------
	wprintf(L"\n");
	struct tm *t = new struct tm;
	time_t timer;
	timer = time(NULL);
	localtime_s(t, &timer);

	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int min = t->tm_min;
	int sec = t->tm_sec;

	while (!m_bShutdown)
	{
		Sleep(1000);
		timer = time(NULL);
		localtime_s(t, &timer);

		if (true == m_bMonitorFlag)
		{
			wprintf(L"	[ServerStart : %d/%d/%d %d:%d:%d]\n\n", year, month, day, hour, min, sec);
			wprintf(L"	[%d/%d/%d %d:%d:%d]\n\n", t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			wprintf(L"	ConnectSession			:	%I64d	\n", m_iConnectClient);
			wprintf(L"	PlayerMapCount			:	%d		\n", GetPlayerCount());
			wprintf(L"	PacketPool_AllocCount		:	%d	\n", CPacket::GetAllocPool());
			wprintf(L"	PlayerPool_AllocCount		:	%d	\n", _PlayerPool->GetAllocCount());
			wprintf(L"	Match_Accept_Total		:	%I64d	\n", m_iAcceptTotal);
			wprintf(L"	Match_Accept_TPS		:	%I64d	\n", m_iAcceptTPS);
			wprintf(L"	Match_SendPacket_TPS		:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	Match_RecvPacket_TPS		:	%I64d	\n\n", m_iRecvPacketTPS);
		}
		m_iAcceptTPS = 0;
		m_iRecvPacketTPS = 0;
		m_iSendPacketTPS = 0;
	}
	delete t;
	return;
}