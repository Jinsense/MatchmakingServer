#ifndef _MATCHINGSERVER_SERVER_PLAYER_H_
#define _MATCHINGSERVER_SERVER_PLAYER_H_

#include <Windows.h>

class CPlayer
{
public:
	CPlayer();
	~CPlayer();

	bool Init();



public:
	unsigned __int64	_Time;		//	Ÿ�Ӿƿ� üũ��
	unsigned __int64	_ClientID;	//	NetServer���� ������ Ŭ���̾�Ʈ ������ȣ
	unsigned __int64	_AccountNo; //	DB�� ����� ���� ������ȣ
	unsigned __int64	_ClientKey; //	�ߺ� ������ ���� Ŭ���̾�Ʈ Ű
	char	_SessionKey[64];		//	�α��� �������� ������ ���� Ű
	in_addr	_ClientIP;				//	Ŭ���̾�Ʈ ���� IP
	int		_Status;

};



#endif

