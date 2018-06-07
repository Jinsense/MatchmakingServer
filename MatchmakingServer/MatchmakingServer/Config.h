#ifndef _MATCHINGSERVER_SERVER_CONFIG_H_
#define _MATCHINGSERVER_SERVER_CONFIG_H_

#include "Parse.h"

class CConfig
{
	enum eNumConfig
	{
		eNUM_BUF = 20,
	};
public:
	CConfig();
	~CConfig();

	bool Set();

public:
	//	NETWORK
	int SERVER_NO;

	WCHAR BIND_IP[20];
	int BIND_IP_SIZE;
	int BIND_PORT;

	WCHAR MASTER_IP[20];
	int MASTER_IP_SIZE;
	int MASTER_PORT;

	WCHAR MONITOR_IP[20];
	int MONITOR_IP_SIZE;
	int MONITOR_PORT;

	int WORKER_THREAD;

	//	SYSTEM
	int CLIENT_MAX;
	int PACKET_CODE;
	int PACKET_KEY1;
	int PACKET_KEY2;
	int LOG_LEVEL;

	//	DATABASE
	char MATCHING_IP[20];
	int MATCHING_IP_SIZE;
	int MATCHING_PORT;
	char MATCHING_USER[20];
	int MATCHING_USER_SIZE;
	char MATCHING_PASSWORD[20];
	int MATCHING_PASSWORD_SIZE;
	char MATCHING_DBNAME[20];
	int MATCHING_DBNAME_SIZE;

	CINIParse _Parse;

private:
	char IP[20];
};

#endif
