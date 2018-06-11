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
	pPlayer->_ClientKey = SET_CLIENTKEY(_Config.SERVER_NO, temp);
	pPlayer->_Time = GetTickCount64();
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.insert(make_pair(pPlayer->_ClientID, pPlayer));
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return;
}

void CMatchServer::OnClientLeave(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	맵에 유저 삭제
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.erase(ClientID);
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
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

		//	JSON 데이터 - AccountNo를 생성한 후 API서버의 select_account.php 요청
		//	JSON 응답을 파싱한 후 세션키를 비교 한다.
		json::value PostData;
		PostData[L"accountno"] = json::value::number(pPlayer->_AccountNo);
		http_client Client(L"http://172.16.2.2:11701/select_account.php");
		Client.request(methods::POST, L"", PostData.to_string().c_str(),
			L"application/json").then([](http_response response)
		{

		});
		//	Config의 버전 코드와 맞는지 비교를 한다.

		//	전부 맞다면 로그인 응답 패킷을 보내준다.

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

		return true;
	}
	
	return true;
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

	return true;
}

bool CMatchServer::InsertPlayer(unsigned __int64 iClientID)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 추가
	//-------------------------------------------------------------
	return true;
}

bool CMatchServer::RemovePlayer(unsigned __int64 iClientID)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 제거
	//-------------------------------------------------------------
	return true;
}

bool CMatchServer::DisconnectPlayer(unsigned __int64 iClientID, BYTE byStatus)
{
	//-------------------------------------------------------------
	//	패킷 전송 후 클라이언트 끊기 - 보내고 끊기
	//	현 컨텐츠에서는 사용 X
	//-------------------------------------------------------------
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
	return;
}