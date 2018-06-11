#include <conio.h>

#include "Config.h"
#include "MatchServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CMatchServer Server;

	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config 파일 불러오기
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
		return false;
	//-----------------------------------------------------------
	// MatchServer Status DB 연결
	//-----------------------------------------------------------
	
	//-----------------------------------------------------------
	// 모니터링 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pMonitor->Connect(Server._Config.MONITOR_IP, Server._Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// 마스터 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pMaster->Connect(Server._Config.MASTER_IP, Server._Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// 매칭 서버 가동
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Server._Config.BIND_IP, Server._Config.BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
		return false;
	//-----------------------------------------------------------
	// Matching status DB에 서버 상태 갱신 - 유저 접속 가능
	//-----------------------------------------------------------
	
	//-----------------------------------------------------------
	// 콘솔 창 제어
	//-----------------------------------------------------------

	return 0;
}