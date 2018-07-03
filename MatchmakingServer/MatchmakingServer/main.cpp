#include <conio.h>

#include "Config.h"
#include "MatchServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CMatchServer Server;
	CPacket::MemoryPoolInit();
	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config 파일 불러오기
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
	{
		wprintf(L"[MatchServer :: Main]	Config Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// Packet Code 설정
	//-----------------------------------------------------------
	CPacket::Init(Server._Config.PACKET_CODE, Server._Config.PACKET_KEY1, Server._Config.PACKET_KEY2);
	//-----------------------------------------------------------
	// MatchServer Status DB 연결
	//-----------------------------------------------------------
	if (false == Server._StatusDB.Set(Server._Config.MATCHING_IP, Server._Config.MATCHING_USER, 
		Server._Config.MATCHING_PASSWORD, Server._Config.MATCHING_DBNAME, Server._Config.MATCHING_PORT))
	{
		wprintf(L"[MatchServer :: Main]	MatchStatus DB Set Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// 모니터링 서버 연결
	//-----------------------------------------------------------
//	if (false == Server._pMonitor->Connect(Server._Config.MONITOR_IP, Server._Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
//	{
//		wprintf(L"[MatchServer :: Main] Monitoring Connect Error\n");
//		return false;
//	}
	//-----------------------------------------------------------
	// 마스터 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pMaster->Connect(Server._Config.MASTER_IP, Server._Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD))
	{
		wprintf(L"[MatchServer :: Main] Master Connect Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// 매칭 서버 가동
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Server._Config.BIND_IP, Server._Config.BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
	{
		wprintf(L"[MatchServer :: Main] MatchingServer Start Error\n]");
		return false;
	}
	//-----------------------------------------------------------
	// Matching status DB에 서버 상태 갱신 - 유저 접속 가능
	//-----------------------------------------------------------
	if(false == Server.MatchDBSet())
	{
		wprintf(L"[MatchServer :: Main] MatchStatus DB Set Error\n]");
		return false;
	}
	//-----------------------------------------------------------
	// 콘솔 창 제어
	//-----------------------------------------------------------
	while (!Server.GetShutDownMode())
	{
		char ch = _getch();
		switch (ch)
		{
		case 'q': case 'Q':
		{
			Server.SetShutDownMode(true);
			wprintf(L"[MatchServer :: Main] Server Shutdown\n");
			break;
		}
		case 'm': case 'M':
		{
			if (false == Server.GetMonitorMode())
			{
				Server.SetMonitorMode(true);
				wprintf(L"[MatchServer :: Main] MonitorMode Start\n");
			}
			else
			{
				Server.SetMonitorMode(false);
				wprintf(L"[MatchServer :: Main] MonitorMode Stop\n");
			}
		}
		}
	}
	return 0;
}