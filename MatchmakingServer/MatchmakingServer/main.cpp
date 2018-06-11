#include <conio.h>

#include "Config.h"
#include "MatchServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CMatchServer Server;

	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config ���� �ҷ�����
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
		return false;
	//-----------------------------------------------------------
	// MatchServer Status DB ����
	//-----------------------------------------------------------
	
	//-----------------------------------------------------------
	// ����͸� ���� ����
	//-----------------------------------------------------------
	if (false == Server._pMonitor->Connect(Server._Config.MONITOR_IP, Server._Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// ������ ���� ����
	//-----------------------------------------------------------
	if (false == Server._pMaster->Connect(Server._Config.MASTER_IP, Server._Config.MASTER_PORT, true, LANCLIENT_WORKERTHREAD))
		return false;
	//-----------------------------------------------------------
	// ��Ī ���� ����
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Server._Config.BIND_IP, Server._Config.BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
		return false;
	//-----------------------------------------------------------
	// Matching status DB�� ���� ���� ���� - ���� ���� ����
	//-----------------------------------------------------------
	
	//-----------------------------------------------------------
	// �ܼ� â ����
	//-----------------------------------------------------------

	return 0;
}