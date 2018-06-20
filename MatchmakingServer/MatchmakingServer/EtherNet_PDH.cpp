#include "EtherNet_PDH.h"


//	PdhEnumObjectItems�� ���ؼ� NetworkInterface �׸񿡼� ���� �� �ִ�
//	�����׸�(Counters) / �������̽� �׸��� ����. �׷��� �� ������ ���̸� �𸣱� ������ 
//	���� ������ ���̸� �˱� ���ؼ� Out Buffer ���ڵ��� NULL �����ͷ� �־ ����� Ȯ��
CEthernet::CEthernet()
{
	PdhOpenQuery(NULL, NULL, &_pdh_Query);
	PdhCollectQueryData(_pdh_Query);
	Init();
	Count();
}

CEthernet::~CEthernet()
{
	delete[] szCounters;
	delete[] szInterfaces;
}

bool CEthernet::Init()
{
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0)
		!= ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return false;
	}
	iCnt = 0;
	szCur = szInterfaces;

	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_EthernetStruct[iCnt]._bUse = true;
		_EthernetStruct[iCnt]._szName[0] = L'\0';

		wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
		//		strcpy_s(_EthernetStruct[iCnt]._szName, szCur);

		szQuery[0] = L'\0';
		StringCchPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		_pdh_Status = PdhAddCounter(_pdh_Query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
		if (_pdh_Status != 0)
			wprintf(L"Format message failed with 0x%x\n", GetLastError());

		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		_pdh_Status = PdhAddCounter(_pdh_Query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
	}
	return true;
}

void CEthernet::Count()
{
	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (true == _EthernetStruct[iCnt]._bUse)
		{
			PdhCollectQueryData(_pdh_Query);
			_pdh_Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &_CounterValue);
			if (_pdh_Status == 0) _pdh_value_Network_RecvBytes += _CounterValue.doubleValue;

			_pdh_Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &_CounterValue);
			if (_pdh_Status == 0) _pdh_value_Network_SendBytes += _CounterValue.doubleValue;
		}
	}
	return;
}

