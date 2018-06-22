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
	//	�ʿ� ���� �߰�
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
	//	�� ���� ��û - true / �� ���� �Ϸ� - false �� ���
	//	������ ������ �� ���� ���� ��Ŷ ����
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
	//	�ʿ� ���� ����
	//-------------------------------------------------------------
	RemovePlayer(ClientID);
	return;
}

void CMatchServer::OnConnectionRequest(WCHAR * pClientID)
{
	//-------------------------------------------------------------
	//	ȭ��Ʈ IP ó��
	//	���� ��� X
	//-------------------------------------------------------------
	return;
}

void CMatchServer::OnError(int ErrorCode, WCHAR *pError)
{
	//-------------------------------------------------------------
	//	���� ó�� - �α� ����
	//	������ ���ǵ� �α� �ڵ带 ���� �α׸� txt�� ����
	//	���� ��� X
	//-------------------------------------------------------------
	return;
}

bool CMatchServer::OnRecv(unsigned __int64 ClientID, CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	����͸� ���� ����
	//-------------------------------------------------------------
	m_iRecvPacketTPS++;
	//-------------------------------------------------------------
	//	���� ó�� - ���� �����ϴ� �������� �˻�
	//-------------------------------------------------------------
	CPlayer *pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		Disconnect(ClientID);
		return false;
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ������ ó��
	//-------------------------------------------------------------
	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ��ġ����ŷ ������ �α��� ��û
	//	Type	: en_PACKET_CS_MATCH_REQ_LOGIN
	//	INT64	: AccountNo
	//	char	: SessionKey[64]
	//	UINT	: Ver_Code
	//	
	//	����	: en_PACKET_CS_MATCH_RES_LOGIN
	//	WORD	: Type
	//	BYTE	: Status
	//-------------------------------------------------------------
	if (en_PACKET_CS_MATCH_REQ_LOGIN == Type)
	{
		UINT Ver_Code;
		*pPacket >> pPlayer->_AccountNo;
		pPacket->PopData((char*)&pPlayer->_SessionKey, sizeof(pPlayer->_SessionKey));
		*pPacket >> Ver_Code;

		//	Config�� ���� �ڵ�� �´��� �񱳸� �Ѵ�.
		if (Ver_Code != _Config.VER_CODE)
		{
			//	���� �ڵ尡 �ٸ� ��� �α׸� ����� �������� ������ ���� �� ���´�.
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

		//	JSON ������ - AccountNo�� ������ �� API������ select_account.php ��û
		//	JSON ������ �Ľ��� �� ����Ű�� �� �Ѵ�.
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
		//	result �� Ȯ��
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
			//	����Ű�� �ٸ� ���� �� ����
			CPacket * newPacket = CPacket::Alloc();
			Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = SESSIONKEY_ERROR;
			*newPacket << Type << Status;
			SendPacketAndDisConnect(pPlayer->_ClientID, newPacket);
			newPacket->Free();
			return true;
		}
		//	�α��� ���� ���� �� ����
		InterlockedIncrement(&_JoinSession);
		//	���� �´ٸ� �α��� ���� ��Ŷ�� �����ش�.
		CPacket * newPacket = CPacket::Alloc();
		Type = en_PACKET_CS_MATCH_RES_LOGIN;
		BYTE Status = LOGIN_SUCCESS;
		*newPacket << Type << Status;
		SendPacket(pPlayer->_ClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - �� ���� ��û
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM
	//	
	//	����	: ������ �������� ClientKey �� �� ���� ��û�� ����
	//			  ������ �������� ������ ���� �� ���� �� 
	//-------------------------------------------------------------
	else if (en_PACKET_CS_MATCH_REQ_GAME_ROOM == Type)
	{
		//	������ ������ ����Ǿ� �ִ��� Ȯ��
		if (false == _pMaster->IsConnect())
		{
			//	�����Ͱ� ���� �ȵǾ� ���� ��� 
			//	�� ���� ��� ���� ��Ŷ Ŭ���̾�Ʈ���� ����
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
			//	�� ���� ��û �÷��� ����
			pPlayer->_bEnterRoomReq = true;
			//	������ ������ �� ���� ��û�� ����
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
	//	��Ŷ ó�� - �� ���� ���� �˸�
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER
	//	WORD	: BattleServerNo
	//	int		: RoomNo
	//
	//	����	: Ŭ���̾�Ʈ�� ��Ʋ���� �� ������ ���� ��
	//			  ������ �������� �� ���� ������ ���� 
	//			  Ŭ���̾�Ʈ���� �� ���� ���� Ȯ�� ��Ŷ ����
	//-------------------------------------------------------------
	else if (en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER == Type)
	{
		//	�� ���� ���� ī��Ʈ ����
		InterlockedIncrement(&_EnterRoomTPS);
		//	�� ���� ���� �÷��� ����
		pPlayer->_bEnterRoomSuccess = true;
		//	������ ������ ����Ǿ� �ִ��� Ȯ��
		if (false == _pMaster->IsConnect())
			return true;
		//	������ ������ �� ���� ������ ����
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
	//	StatusDB�� �����ð����� ���� ��ġ���� Timestamp �� ����
	//	��ü ���� ������� Heartbeat �������� TimeOut üũ 
	//	���� ������ ���� �̻� ��ȭ�� �߻����� �� DB�� connectuser �� ����
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
				//	�ٷ� Disconnect �ϸ� ����� ���輺
				//	�ӽ� stack�� �ְ� �������� Disconnect ȣ���� ��
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
	//	����͸� ������ ������ �׸� ������Ʈ �� ����
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(1000);
		if (false == _pMonitor->IsConnect())
		{
			//	���� ���°� �ƴҰ�� ������ �õ�
			_pMonitor->Connect(_Config.MONITOR_IP, _Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
		PdhCollectQueryData(_CpuQuery);
		PdhGetFormattedCounterValue(_MemoryNonpagedBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_Nonpaged_Memory = (int)_CounterVal.doubleValue / (1024 * 1024);
		PdhGetFormattedCounterValue(_MemoryAvailableMBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_Available_Memory = (int)_CounterVal.doubleValue;
		PdhGetFormattedCounterValue(_ProcessPrivateBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_MatchServer_Memory_Commit = (int)_CounterVal.doubleValue / (1024 * 1024);
		//-------------------------------------------------------------
		//	����͸� ������ ������ ��Ŷ ���� �� ����
		//-------------------------------------------------------------
		// �ϵ���� CPU ���� ��ü
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL);
		// �ϵ���� ��밡�� �޸�
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY);
		// �ϵ���� �̴��� ���� kb
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV);
		// �ϵ���� �̴��� �۽� kb
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND);
		// �ϵ���� �������� �޸� ��뷮
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY);
		//	��ġ����ŷ ���� ON
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_SERVER_ON);
		//	��ġ����ŷ CPU ���� ( Ŀ�� + ���� )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_CPU);
		//	��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT);
		//	��ġ����ŷ ��ŶǮ ��뷮
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL);
		//	��ġ����ŷ ���� ����
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_SESSION);
		//	��ġ����ŷ ���� ���� ( �α��� ���� �� )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_PLAYER);
		//	��ġ����ŷ �� ���� ���� �� ( �ʴ� )     
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS);
	}
	return;
}

void CMatchServer::LanMasterCheckThead_Update()
{
	//-------------------------------------------------------------
	//	������ ���� ���� �ֱ������� Ȯ��
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
	//	Config ���� �ҷ�����
	//-------------------------------------------------------------
	_Config.Set();
	return;
}

void CMatchServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF8 Ÿ���� UTF16���� ��ȯ
	//-------------------------------------------------------------
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CMatchServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF16 Ÿ���� UTF8�� ��ȯ
	//-------------------------------------------------------------
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}

bool CMatchServer::MatchDBSet()
{
	//-------------------------------------------------------------
	//	Matchmaking Status DB�� ��Ī���� ���� insert / update
	//-------------------------------------------------------------
	//	���� �ڽ��� IP�� ���Ѵ�.
	IN_ADDR myip = GetMyIPAddress();

	//	���� ������ȣ�� �ִ� ��� INSERT ���� �߻�
	bool bRes = _StatusDB.Query_Save(L"INSERT INTO `server` (`serverno`, `ip`, `port`, `connectuser`, `heartbeat` ) VALUES (%d, %s, $d, NOW())", _Config.SERVER_NO, myip, _Config.BIND_PORT, GetPlayerCount());
	if (false == bRes)
	{
		bRes = _StatusDB.Query_Save(L"UPDATE `server` set `ip` = %s, `port` = %d, `connectuser` = %d, `heartbeat` = NOW() where `serverno` = %d", myip, _Config.BIND_PORT, GetPlayerCount(), _Config.SERVER_NO);
		if (false == bRes)
		{
			//	Status DB�� ��ġ���� ��� ����
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"STATUS DB INSERT/UPDATE FAIL"));
			g_CrashDump->Crash();
		}
	}
	//-------------------------------------------------------------
	//	��Ʈ��Ʈ ������ ����
	//-------------------------------------------------------------
	_hHeartbeatThread = (HANDLE)_beginthreadex(NULL, 0, &HeartbeatThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	HeartbeatThread Create\n");

	//-------------------------------------------------------------
	//	����͸� ���� ������ ����
	//-------------------------------------------------------------
	_hLanMonitorThread = (HANDLE)_beginthreadex(NULL, 0, &HeartbeatThread,
		(LPVOID)this, 0, NULL);
	wprintf(L"[Server :: Server_Start]	HeartbeatThread Create\n");
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
	//	�ϵ���� CPU ���� ��ü
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
	//	�ϵ���� ��밡�� �޸�
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
	//	�ϵ���� �̴��� ���� kb
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
	//	�ϵ���� �̴��� �۽� kb
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
	//	�ϵ���� �������� �޸� ��뷮 kb
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
	//	��ġ����ŷ ���� On
	//-------------------------------------------------------------
	case dfMONITOR_DATA_TYPE_MATCH_SERVER_ON:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MatchServer_On << _TimeStamp;
	}
	break;
	//-------------------------------------------------------------
	//	��ġ����ŷ CPU ���� (Ŀ�� + ����)
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
	//	��ġ����ŷ �޸� ���� Ŀ�� ��뷮 (Private) MByte
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
	//	��ġ����ŷ ��ŶǮ ��뷮
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
	//	��ġ����ŷ ���� ����
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
	//	��ġ����ŷ ���� ���� (�α��� ���� ��)
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
	//	��ġ����ŷ �� ���� ���� �� (�ʴ�)
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
	//	Player Map�� Ŭ���̾�Ʈ �߰�
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.insert(make_pair(ClientID, pPlayer));
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CMatchServer::RemovePlayer(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	Player Map�� Ŭ���̾�Ʈ ����
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.erase(ClientID);
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CMatchServer::DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo)
{
	//-------------------------------------------------------------
	//	TimeOut ���� ����
	//	Log ����� Disconnect ȣ��
	//-------------------------------------------------------------
	_pLog->Log(const_cast<WCHAR*>(L"TimeOut"), LOG_SYSTEM,
		const_cast<WCHAR*>(L"TimeOut [AccountNo : %d]"), AccountNo);
	Disconnect(ClientID);
	return true;
}

int CMatchServer::GetPlayerCount()
{
	//-------------------------------------------------------------
	//	���� ���� ���� �÷��̾� �� ���
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
	//	����͸� ������ - NetServer���� �����Լ� ��ӹ޾� ���
	//	����͸� �� �׸��� 1�� ������ �����Ͽ� �ܼ� ȭ�鿡 ���
	//
	//	����͸� �׸��
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
	/*		wprintf(L"	ConnectSession			:	%I64d	\n", m_iConnectClient);
			wprintf(L"	PlayerMapCount			:	%d		\n", GetPlayerCount());
			wprintf(L"	PacketPool_AllocCount		:	%d	\n", CPacket::GetAllocPool());
			wprintf(L"	PlayerPool_AllocCount		:	%d	\n", _PlayerPool->GetAllocCount());
			wprintf(L"	Match_Accept_Total		:	%I64d	\n", m_iAcceptTotal);
			wprintf(L"	Match_Accept_TPS		:	%I64d	\n", m_iAcceptTPS);
			wprintf(L"	Match_SendPacket_TPS		:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	Match_RecvPacket_TPS		:	%I64d	\n\n", m_iRecvPacketTPS);*/
			wprintf(L"	�����ڼ�			:	%I64d	\n", m_iConnectClient);
			wprintf(L"	�÷��̾�� �ο�			:	%d		\n", GetPlayerCount());
			wprintf(L"	�α��� ���� �ο�		:	%d		\n", _JoinSession);
			wprintf(L"	�� ���� ���� 1�ʴ� Ƚ��		:%d		\n", _EnterRoomTPS);
			wprintf(L"	��ŶǮ Alloc		:	%d	\n", CPacket::GetAllocPool());
			wprintf(L"	�÷��̾�Ǯ Alloc		:	%d	\n", _PlayerPool->GetAllocCount());
			wprintf(L"	��ġ���� Accept �� Ƚ��		:	%I64d	\n", m_iAcceptTotal);
			wprintf(L"	��ġ���� Accept 1�ʴ� Ƚ��		:	%I64d	\n", m_iAcceptTPS);
			wprintf(L"	��ġ���� Send 1�ʴ� KByte		:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	��ġ���� Recv 1�ʴ� KByte		:	%I64d	\n", m_iRecvPacketTPS);
			wprintf(L"	�������� �޸� ũ�� MByte		:	%d		\n", _Nonpaged_Memory);			
			wprintf(L"	CPU ����				:	%d		\n\n", _MatchServer_CPU);
		}
		m_iAcceptTPS = 0;
		m_iRecvPacketTPS = 0;
		m_iSendPacketTPS = 0;
	}
	delete t;
	return;
}