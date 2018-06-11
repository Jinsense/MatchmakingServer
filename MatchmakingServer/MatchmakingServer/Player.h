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
	//-------------------------------------------------------------
	// JSON
	//-------------------------------------------------------------
	StringBuffer StringJSON;
	Writer<StringBuffer, UTF16<>> wirter(StringJSON);
	char *		_pJson;

	unsigned __int64	_Time;		//	타임아웃 체크용
	unsigned __int64	_ClientID;	//	NetServer에서 생성한 클라이언트 고유번호
	INT64	_AccountNo; //	DB에 저장된 계정 고유번호
	unsigned __int64	_ClientKey; //	중복 방지용 고유 클라이언트 키
	char	_SessionKey[64];		//	로그인 서버에서 생성한 세션 키
	in_addr	_ClientIP;				//	클라이언트 접속 IP
	int		_Status;

};



#endif

