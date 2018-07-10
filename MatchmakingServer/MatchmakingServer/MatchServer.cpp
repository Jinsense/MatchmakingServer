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
	_pLog = _pLog->GetInstance();

	_JoinSession = NULL;
	_EnterRoomTPS = NULL;
	_TimeStamp = NULL;
	_CPU_Total = NULL;
	_Available_Memory = NULL;
	_Network_Send = NULL;
	_Nonpaged_Memory = NULL;
	_MatchServer_On = 1;
	_MatchServer_CPU = NULL;
	_MatchServer_PacketPool = NULL;
	_MatchServer_Session = NULL;
	_MatchServer_Player = NULL;
	_MatchServer_MatchSuccess = NULL;

	PdhOpenQuery(NULL, NULL, &_CpuQuery);
	PdhAddCounter(_CpuQuery, L"\\Memory\\Available MBytes", NULL, &_MemoryAvailableMBytes);
	PdhAddCounter(_CpuQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_MemoryNonpagedBytes);
	PdhAddCounter(_CpuQuery, L"\\Process(MMOGameServer)\\Private Bytes", NULL, &_ProcessPrivateBytes);
	PdhCollectQueryData(_CpuQuery);
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
	pPlayer->_ClientKey = pPlayer->_ClientID;
	__int64 serverno = _Config.SERVER_NO;
	strcpy_s(pPlayer->_ClientIP, sizeof(pPlayer->_ClientIP), Info.IP);
	SET_CLIENTKEY(serverno, pPlayer->_ClientKey);
	pPlayer->_Time = GetTickCount64();
	InsertPlayer(pPlayer->_ClientID, pPlayer);
	return;
}

void CMatchServer::OnClientLeave(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	방 입장 요청 - true / 방 입장 완료 - false 일 경우
	//	마스터 서버에 방 입장 실패 패킷 전송
	//-------------------------------------------------------------
	CPlayer * pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"FindClientID Fail [ClientID : %d]"), ClientID);
		return;
	}
	if (true == pPlayer->_bEnterRoomReq && false == pPlayer->_bEnterRoomSuccess)
	{
		CPacket *pPacket = CPacket::Alloc();
		WORD Type = en_PACKET_MAT_MAS_REQ_ROOM_ENTER_FAIL;
		*pPacket << Type << pPlayer->_ClientKey;
		_pMaster->SendPacket(pPacket);
		pPacket->Free();
	}
	//-------------------------------------------------------------
	//	맵에 유저 삭제
	//-------------------------------------------------------------
	RemovePlayer(ClientID);
	_PlayerPool->Free(pPlayer);
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
		pPacket->PopData((char*)&pPlayer->_SessionKey[0], sizeof(pPlayer->_SessionKey));
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
		//-------------------------------------------------------------
		//	JSON 데이터 - AccountNo를 생성한 후 API서버의 select_account.php 요청
		//	JSON 응답을 파싱한 후 세션키를 비교 한다.
		//-------------------------------------------------------------
		//	Set Post Data
		Json::Value PostData;
		Json::StyledWriter writer;
		PostData["accountno"] = pPlayer->_AccountNo;
		string Data = writer.write(PostData);
//		WinHttpClient HttpClient(L"http://192.168.10.117:11701/select_account.php");
		WinHttpClient HttpClient(_Config.APISERVER_IP);
		HttpClient.SetAdditionalDataToSend((BYTE*)Data.c_str(), Data.size());
		
		//	Set Request Header
		wchar_t szSize[50] = L"";
		swprintf_s(szSize, L"%d", Data.size());
		wstring Headers = L"Content-Length: ";
		Headers += szSize;
		Headers += L"\r\nContent-Type: application/x-www-form-urlencoded\r\n";
		HttpClient.SetAdditionalRequestHeaders(Headers);

		//	Sent HTTP post request
		HttpClient.SendHttpRequest(L"POST");

		//	Response wstring -> string convert
		wstring response = HttpClient.GetResponseContent();
		string temp;
		temp.assign(response.begin(), response.end());

		//	result 값 확인
		Json::Reader reader;
		Json::Value result;
		bool Res = reader.parse(temp, result);
		if (!Res)
		{
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"Failed to parse Json [AccountNo : %d]"), pPlayer->_AccountNo);
			CPacket * newPacket = CPacket::Alloc();
			Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = ETC_ERROR;
			*newPacket << Type << Status;
			SendPacketAndDisConnect(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}
		int ResNum = result["result"].asInt();
		if (SUCCESS != ResNum)
		{
			if (NOT_JOIN == ResNum)
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

		string sessionkey = result["sessionkey"].asCString();
		if (0 != strncmp(sessionkey.c_str(), pPlayer->_SessionKey, sizeof(pPlayer->_SessionKey)))
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
		//	로그인 성공 유저 수 증가
		InterlockedIncrement(&_JoinSession);
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
		//	마스터 서버가 연결되어 있는지 확인
		if (false == _pMaster->IsConnect())
		{
			//	마스터가 연결 안되어 있을 경우 
			//	방 정보 얻기 실패 패킷 클라이언트에게 전송
			Type = en_PACKET_CS_MATCH_RES_GAME_ROOM;
			BYTE Status = 0;
			CPacket *newPacket = CPacket::Alloc();
			*newPacket << Type << Status;
			SendPacket(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}
		else
		{
			//	방 배정 요청 플래그 변경
			pPlayer->_bEnterRoomReq = true;
			//	마스터 서버에 방 정보 요청을 보냄
			CPacket *newPacket = CPacket::Alloc();
			Type = en_PACKET_MAT_MAS_REQ_GAME_ROOM;
			*newPacket << Type << pPlayer->_ClientKey << pPlayer->_AccountNo;
			_pMaster->SendPacket(newPacket);
			newPacket->Free();
			return true;
		}
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
	//			  클라이언트에게 방 입장 성공 확인 패킷 전달
	//-------------------------------------------------------------
	else if (en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER == Type)
	{
		//	방 배정 성공 카운트 증가
		InterlockedIncrement(&_EnterRoomTPS);
		//	방 배정 성공 플래그 변경
		pPlayer->_bEnterRoomSuccess = true;
		//	마스터 서버가 연결되어 있는지 확인
		if (false == _pMaster->IsConnect())
			return true;
		//	마스터 서버에 방 정보 성공을 보냄
		WORD BattleServerNo;
		int RoomNo;
		*pPacket >> BattleServerNo >> RoomNo;
		
		CPacket *newPacket = CPacket::Alloc();
		Type = en_PACKET_MAT_MAS_REQ_ROOM_ENTER_SUCCESS;
		*newPacket << Type << BattleServerNo << RoomNo << pPlayer->_ClientKey;
		_pMaster->SendPacket(newPacket);
		newPacket->Free();
		
		CPacket * resPacket = CPacket::Alloc();
		Type = en_PACKET_CS_MATCH_RES_GAME_ROOM_ENTER;
		*resPacket << Type;
		SendPacket(pPlayer->_ClientID, resPacket);
		resPacket->Free();
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
	std::stack<unsigned __int64> temp;

	while (1)
	{
		Sleep(1000);
		UINT64 now = GetTickCount64();
		if (now - start > _Config.DB_TIME_UPDATE)
		{
			if (false == _StatusDB.Query(L"update `server` set `heartbeat` = now() where serverno = %d", _Config.SERVER_NO))
				_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB HEARTBEAT UPDATE FAIL [ServerNo : %d]"), _Config.SERVER_NO);
			start = now;
		}
		/*
		AcquireSRWLockExclusive(&_PlayerMap_srwlock);
		for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
		{
			if (now - i->second->_Time > _Config.USER_TIMEOUT)
			{
				_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"USER TIMEOUT : %d [AccountNo : %d]"), now - i->second->_Time, i->second->_AccountNo);
				//	바로 Disconnect 하면 데드락 위험성
				//	임시 stack에 넣고 마지막에 Disconnect 호출할 것
				temp.push(i->second->_ClientID);
			}
		}
		ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
		while (!temp.empty())
		{
			unsigned __int64 ClientID = temp.top();
			Disconnect(ClientID);
			temp.pop();
		}
		*/
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

void CMatchServer::LanMonitoringThread_Update()
{
	//-------------------------------------------------------------
	//	모니터링 서버에 전송할 항목 업데이트 후 전송
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(1000);
		PdhCollectQueryData(_CpuQuery);
		PdhGetFormattedCounterValue(_MemoryNonpagedBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_Nonpaged_Memory = (int)_CounterVal.doubleValue / (1024 * 1024);
		PdhGetFormattedCounterValue(_MemoryAvailableMBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_Available_Memory = (int)_CounterVal.doubleValue;
		PdhGetFormattedCounterValue(_ProcessPrivateBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_MatchServer_Memory_Commit = (int)_CounterVal.doubleValue / (1024 * 1024);
		if (false == _pMonitor->IsConnect())
		{
			//	연결 상태가 아닐경우 재접속 시도
			_pMonitor->Connect(_Config.MONITOR_IP, _Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
		//-------------------------------------------------------------
		//	모니터링 서버에 전송할 패킷 생성 후 전송
		//-------------------------------------------------------------
		// 하드웨어 CPU 사용률 전체
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL);
		// 하드웨어 사용가능 메모리
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY);
		// 하드웨어 이더넷 수신 kb
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV);
		// 하드웨어 이더넷 송신 kb
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND);
		// 하드웨어 논페이지 메모리 사용량
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY);
		//	매치메이킹 서버 ON
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_SERVER_ON);
		//	매치메이킹 CPU 사용률 ( 커널 + 유저 )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_CPU);
		//	매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT);
		//	매치메이킹 패킷풀 사용량
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL);
		//	매치메이킹 접속 세션
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_SESSION);
		//	매치메이킹 접속 유저 ( 로그인 성공 후 )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_PLAYER);
		//	매치메이킹 방 배정 성공 수 ( 초당 )     
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS);
	}
	return;
}

void CMatchServer::LanMasterCheckThead_Update()
{
	//-------------------------------------------------------------
	//	마스터 서버 연결 주기적으로 확인
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(5000);
		if (false == _pMaster->IsConnect())
		{
			_pMaster->Connect(_Config.MASTER_IP, _Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
	}
	return;
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
	//IN_ADDR myip = GetMyIPAddress();
	//char *temp = inet_ntoa(myip);
	//WCHAR IP[20] = { 0, };
	//UTF8toUTF16(temp, IP, sizeof(IP));
	//	기존 서버번호가 있는 경우 INSERT 에러 발생
	bool bRes = _StatusDB.Query(L"INSERT INTO `server` (`serverno`, `ip`, `port`, `connectuser`, `heartbeat` ) VALUES (%d, '%s', %d, %d, NOW())", _Config.SERVER_NO, _Config.MATCH_IP, _Config.BIND_PORT, GetPlayerCount());
	if (false == bRes)
	{
		bRes = _StatusDB.Query(L"UPDATE `server` set `ip` = '%s', `port` = %d, `connectuser` = %d, `heartbeat` = NOW() where `serverno` = %d;", _Config.MATCH_IP, _Config.BIND_PORT, GetPlayerCount(), _Config.SERVER_NO);
		if (false == bRes)
		{
			//	Status DB에 매치서버 등록 실패
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB INSERT/UPDATE FAIL"));
			g_CrashDump->Crash();
		}
	}
	//-------------------------------------------------------------
	//	하트비트 스레드 생성
	//-------------------------------------------------------------
	_hHeartbeatThread = (HANDLE)_beginthreadex(NULL, 0, &HeartbeatThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	HeartbeatThread Create\n");

	//-------------------------------------------------------------
	//	모니터링 전송 스레드 생성
	//-------------------------------------------------------------
	_hLanMonitorThread = (HANDLE)_beginthreadex(NULL, 0, &LanMonitoringThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	HeartbeatThread Create\n");

	//-------------------------------------------------------------
	//	마스터 서버 연결 체크 스레드 생성
	//-------------------------------------------------------------
	_hLanMasterThread = (HANDLE)_beginthreadex(NULL, 0, &LanMasterCheckThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	LanMasterCheckThread Create\n");
	return true;
}

bool CMatchServer::LanMonitorSendPacket(BYTE DataType)
{
	struct tm *pTime = new struct tm;
	time_t Now;
	localtime_s(pTime, &Now);
	_TimeStamp = time(NULL);
	WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

	switch (DataType)
	{
	//-------------------------------------------------------------
	//	하드웨어 CPU 사용률 전체
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL:
	{
		_CPU_Total = _Cpu.ProcessorTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _CPU_Total << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	하드웨어 사용가능 메모리
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _Available_Memory << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	하드웨어 이더넷 수신 kb
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV:
	{
		_Network_Recv = _Ethernet._pdh_value_Network_RecvBytes / (1024);
		_Ethernet._pdh_value_Network_RecvBytes = 0;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _Network_Recv << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	하드웨어 이더넷 송신 kb
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND:
	{
		_Network_Send = _Ethernet._pdh_value_Network_SendBytes / (1024);
		_Ethernet._pdh_value_Network_SendBytes = 0;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _Network_Send << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	하드웨어 논페이지 메모리 사용량 kb
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _Nonpaged_Memory << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 서버 On
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_SERVER_ON:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_On << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 CPU 사용률 (커널 + 유저)
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_CPU:
	{
		_MatchServer_CPU = _Cpu.ProcessTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_CPU << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_Memory_Commit << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();		
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 패킷풀 사용량
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL:
	{
		_MatchServer_PacketPool = CPacket::m_pMemoryPool->_UseCount;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_PacketPool << _TimeStamp;
		pPacket->Free();
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 접속 세션
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_SESSION:
	{
		_MatchServer_Session = GetPlayerCount();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_Session << _TimeStamp;
		pPacket->Free();		
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 접속 유저 (로그인 성공 후)
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_PLAYER:
	{
		_MatchServer_Player = _JoinSession;
		_JoinSession = 0;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_Player << _TimeStamp;
		pPacket->Free();		
	}
	break;
	//-------------------------------------------------------------
	//	매치메이킹 방 배정 성공 수 (초당)
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS:
	{
		_MatchServer_MatchSuccess = _EnterRoomTPS;
		_EnterRoomTPS = 0;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_MatchSuccess << _TimeStamp;
		pPacket->Free();
	}
	break;
	default:
		break;
	}
	delete pTime;
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

unsigned __int64 CMatchServer::FindPlayer_ClientKey(UINT64 ClientKey)
{
	unsigned __int64 ClientID = NULL;
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
	{
		if (i->second->_ClientKey == ClientKey)
		{
			ClientID = i->second->_ClientID;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return ClientID;
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
		_Cpu.UpdateCpuTime();
		_Ethernet.Count();
		timer = time(NULL);
		localtime_s(t, &timer);

		if (true == m_bMonitorFlag)
		{
			wprintf(L"	[ServerStart : %d/%d/%d %d:%d:%d]\n\n", year, month, day, hour, min, sec);
			wprintf(L"	[%d/%d/%d %d:%d:%d]\n\n", t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	
			wprintf(L"	Connect User			:	%I64d	\n", m_iConnectClient);
			wprintf(L"	PlayerMap User			:	%d		\n", GetPlayerCount());
			wprintf(L"	LoginSuccess User Total		:	%d		\n", _JoinSession);
			wprintf(L"	EnterRoomTPS			:	%d		\n", _EnterRoomTPS);
			wprintf(L"	PacketPool Alloc		:	%d	\n", CPacket::GetAllocPool());
			wprintf(L"	PacketPool Use			:	%d	\n", CPacket::_UseCount);
			wprintf(L"	PlayerPool Alloc		:	%d	\n", _PlayerPool->GetAllocCount());
			wprintf(L"	PlayerPool User			:	%d	\n\n", _PlayerPool->GetUseCount());
			wprintf(L"	MatchServer Accept Total	:	%I64d	\n", m_iAcceptTotal);
			wprintf(L"	MatchServer Accept TPS		:	%I64d	\n", m_iAcceptTPS);
			wprintf(L"	MatchServer Send KByte/s	:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	MatchServer Recv KByte/s	:	%I64d	\n", m_iRecvPacketTPS);
			wprintf(L"	NonPaged Memory MByte		:	%d		\n", _Nonpaged_Memory);			
			wprintf(L"	CPU Usage			:	%d		\n\n", _MatchServer_CPU);
		}
		m_iAcceptTPS = 0;
		m_iRecvPacketTPS = 0;
		m_iSendPacketTPS = 0;
	}
	delete t;
	return;
}