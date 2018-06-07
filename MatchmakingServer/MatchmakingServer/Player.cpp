#include "Player.h"

CPlayer::CPlayer()
{
	Init();
}

CPlayer::~CPlayer()
{

}

bool CPlayer::Init()
{
	_Time = NULL;
	_ClientID = NULL;
	_AccountNo = NULL;
	_ClientKey = NULL;
	_Status = NULL;
	ZeroMemory(_SessionKey, sizeof(_SessionKey));
	return true;
}