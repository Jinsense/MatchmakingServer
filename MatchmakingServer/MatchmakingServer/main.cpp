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
	// Config ���� �ҷ�����
	//-----------------------------------------------------------
	if (false == Config.Set())
		return false;
	//-----------------------------------------------------------
	// ����͸� ���� ����
	//-----------------------------------------------------------
	if (false == Server._pMonitor->Connect(Config.MONITOR_IP, Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// ������ ���� ����
	//-----------------------------------------------------------
	if (false == Server._pMaster->Connect(Config.MASTER_IP, Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// ��Ī ���� ����
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Config.BIND_IP, Config.BIND_PORT, Config.WORKER_THREAD, true, Config.CLIENT_MAX))
		return false;
	//-----------------------------------------------------------
	// Matching status DB�� ���� ���� ���� - ���� ���� ����
	//-----------------------------------------------------------


}