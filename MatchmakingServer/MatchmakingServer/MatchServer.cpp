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


}

CMatchServer::~CMatchServer()
{

}

void CMatchServer::OnClientJoin(st_SessionInfo Info)
{
	//-------------------------------------------------------------
	//	맵에 유저 추가
	//-------------------------------------------------------------

	return;
}

void CMatchServer::OnClientLeave(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	맵에 유저 삭제
	//-------------------------------------------------------------


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
	//	패킷 처리 - 컨텐츠 처리
	//-------------------------------------------------------------
	
	//-------------------------------------------------------------
	//	모니터링 측정 변수
	//-------------------------------------------------------------
	m_iRecvPacketTPS++;

	//-------------------------------------------------------------
	//	Player Map에서 사용자를 찾음
	//	없는 유저일 경우 로그 남기고 해당 유저 끊음
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//	패킷 처리 - 패킷 Type 확인
	//-------------------------------------------------------------

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

	//-------------------------------------------------------------
	//	패킷 처리 - 방 정보 요청
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM
	//	
	//	응답	: 마스터 서버에게 ClientKey 와 방 정보 요청을 보냄
	//			  마스터 서버에게 정보를 받은 후 돌려 줌 
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//	패킷 처리 - 방 입장 성공 알림
	//	Type	: en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER
	//	WORD	: BattleServerNo
	//	int		: RoomNo
	//
	//	응답	: 클라이언트가 배틀서버 방 입장을 성공 함
	//			  마스터 서버에게 방 입장 성공을 전달 
	//-------------------------------------------------------------

	
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