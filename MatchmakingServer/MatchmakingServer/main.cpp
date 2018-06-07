#include <conio.h>

#include "Config.h"
#include "MatchServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CConfig Config;
	CMatchServer Server;

	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config 파일 불러오기
	//-----------------------------------------------------------
	if (false == Config.Set())
		return false;
	//-----------------------------------------------------------
	// 모니터링 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pMonitor->Connect(Config.MONITOR_IP, Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// 마스터 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pMaster->Connect(Config.MASTER_IP, Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// 매칭 서버 가동
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Config.BIND_IP, Config.BIND_PORT, Config.WORKER_THREAD, true, Config.CLIENT_MAX))
		return false;
	//-----------------------------------------------------------
	// Matching status DB에 서버 상태 갱신 - 유저 접속 가능
	//-----------------------------------------------------------


}