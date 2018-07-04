#include <windows.h>

#include "Config.h"

CConfig::CConfig()
{
	SERVER_NO = NULL;
	VER_CODE = NULL;
	ZeroMemory(&MASTERTOKEN, sizeof(MASTERTOKEN));
	MASTERTOKEN_SIZE = eNUM_BUF * 2;

	ZeroMemory(&BIND_IP, sizeof(BIND_IP));
	BIND_IP_SIZE = eNUM_BUF;
	BIND_PORT = NULL;

	ZeroMemory(&MASTER_IP, sizeof(MASTER_IP));
	MASTER_IP_SIZE = eNUM_BUF;
	MASTER_PORT = NULL;

	ZeroMemory(&APISERVER_IP, sizeof(APISERVER_IP));
	APISERVER_IP_SIZE = 3 * eNUM_BUF;

	WORKER_THREAD = NULL;
	DB_TIME_UPDATE = NULL;
	USER_TIMEOUT = NULL;
	USER_CHANGE = NULL;
	CLIENT_MAX = NULL;
	PACKET_CODE = NULL;
	PACKET_KEY1 = NULL;
	PACKET_KEY2 = NULL;
	LOG_LEVEL = NULL;

	ZeroMemory(&MATCHING_IP, sizeof(MATCHING_IP));
	MATCHING_IP_SIZE = eNUM_BUF;
	MATCHING_PORT = NULL;
	ZeroMemory(&MATCHING_USER, sizeof(MATCHING_USER));
	MATCHING_USER_SIZE = eNUM_BUF;
	ZeroMemory(&MATCHING_PASSWORD, sizeof(MATCHING_PASSWORD));
	MATCHING_PASSWORD_SIZE = eNUM_BUF;
	ZeroMemory(&MATCHING_DBNAME, sizeof(MATCHING_DBNAME));
	MATCHING_DBNAME_SIZE = eNUM_BUF;

	ZeroMemory(&IP, sizeof(IP));
}

CConfig::~CConfig()
{

}

bool CConfig::Set()
{
	bool res = true;
	res = _Parse.LoadFile(L"MatchingServer_Config.ini");
	if (false == res)
		return false;
	res = _Parse.ProvideArea("NETWORK");
	if (false == res)
		return false;
	res = _Parse.GetValue("MASTERTOKEN", &MASTERTOKEN[0], &MASTERTOKEN_SIZE);
	if (false == res)
		return false;
	res = _Parse.GetValue("SERVER_NO", &SERVER_NO);
	if (false == res)
		return false;
	res = _Parse.GetValue("VER_CODE", &VER_CODE);
	if (false == res)
		return false;
	
	res = _Parse.GetValue("BIND_IP", &IP[0], &BIND_IP_SIZE);
	if (false == res)
		return false;
	_Parse.UTF8toUTF16(IP, BIND_IP, sizeof(BIND_IP));
	res = _Parse.GetValue("BIND_PORT", &BIND_PORT);
	if (false == res)
		return false;
	_Parse.GetValue("MASTER_IP", &IP[0], &MASTER_IP_SIZE);
	_Parse.UTF8toUTF16(IP, MASTER_IP, sizeof(MASTER_IP));
	_Parse.GetValue("MASTER_PORT", &MASTER_PORT);
	res = _Parse.GetValue("APISERVER_IP", &IP[0], &APISERVER_IP_SIZE);
	_Parse.UTF8toUTF16(IP, APISERVER_IP, sizeof(APISERVER_IP));
	if (false == res)
		return false;

	_Parse.ProvideArea("SYSTEM");
	res = _Parse.GetValue("WORKER_THREAD", &WORKER_THREAD);
	if (false == res)
		return false;
	_Parse.GetValue("DB_TIME_UPDATE", &DB_TIME_UPDATE);
	_Parse.GetValue("USER_TIMEOUT", &USER_TIMEOUT);
	_Parse.GetValue("USER_CHANGE", &USER_CHANGE);
	_Parse.GetValue("CLIENT_MAX", &CLIENT_MAX);
	_Parse.GetValue("PACKET_CODE", &PACKET_CODE);
	_Parse.GetValue("PACKET_KEY1", &PACKET_KEY1);
	_Parse.GetValue("PACKET_KEY2", &PACKET_KEY2);
	_Parse.GetValue("LOG_LEVEL", &LOG_LEVEL);

	_Parse.ProvideArea("DATABASE");
	_Parse.GetValue("MATCHING_IP", &MATCHING_IP[0], &MATCHING_IP_SIZE);
	_Parse.GetValue("MATCHING_PORT", &MATCHING_PORT);
	_Parse.GetValue("MATCHING_USER", &MATCHING_USER[0], &MATCHING_USER_SIZE);
	_Parse.GetValue("MATCHING_PASSWORD", &MATCHING_PASSWORD[0], &MATCHING_PASSWORD_SIZE);
	_Parse.GetValue("MATCHING_DBNAME", &MATCHING_DBNAME[0], &MATCHING_DBNAME_SIZE);

	return true;
}