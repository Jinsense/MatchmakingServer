#ifndef _MASTERSERVER_LIB_ETHERNET_H_
#define _MASTERSERVER_LIB_ETHERNET_H_

#include <Windows.h>
#include <Pdh.h>
#include <Strsafe.h>

#define df_PDH_ETHERNET_MAX			8

#pragma comment(lib,"Pdh.lib")
//	�̴��� �ϳ��� ���� Send, Recv PDH ���� ����
struct st_ETHERNET
{
	bool	_bUse;
	WCHAR	_szName[128];

	PDH_HCOUNTER	_pdh_Counter_Network_RecvBytes;
	PDH_HCOUNTER	_pdh_Counter_Network_SendBytes;
};

class CEthernet
{
public:
	CEthernet();
	~CEthernet();

	bool Init();
	void Count();

public:

	PDH_STATUS		_pdh_Status;
	PDH_HQUERY		_pdh_Query;
	st_ETHERNET		_EthernetStruct[df_PDH_ETHERNET_MAX];	//	��ī�庰 PDH ����
	double			_pdh_value_Network_RecvBytes;			//	�� Recv Bytes ��� �̴����� Recv ��ġ �ջ�
	double			_pdh_value_Network_SendBytes;			//	�� Send Bytes ��� �̴����� Send ��ġ �ջ�

private:
	int iCnt = 0;
	bool bErr = false;
	WCHAR *szCur = NULL;
	WCHAR *szCounters = NULL;
	WCHAR *szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };

	PDH_FMT_COUNTERVALUE _CounterValue;
};

#endif // !_SERVER_MONITOR_ETHERNET_H_





