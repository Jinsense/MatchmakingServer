#ifndef _MATCHINGSERVER_LIB_JSON_H_
#define _MATCHINGSERVER_LIB_JSON_H_

#include "../json/include/rapidjson/document.h"
#include "../json/include/rapidjson/writer.h"
#include "../json/include/rapidjson/stringbuffer.h"

using namespace rapidjson;

StringBuffer StringJSON;
Writer<StringBuffer, UTF16<>> writer(StringJSON);


#include <cpprest\json.h>

class CJsonObject
{
public:
	CJsonObject();
	CJsonObject(web::json::value _pValue, std::vector<std::wstring> _pKeyList);

	bool	hasStr()	{ return !_StrList.empty(); }
	bool	hasInt()	{ return !_IntList.empty(); }
	bool	hasDbl()	{ return !_DblList.empty(); }
	bool	hasBool()	{ return !_BoolList.empty(); }
	bool	isArray()	{ return !_vArrayList.empty(); }

	std::wstring	GetString(std::wstring _pKey)	{ return _StrList.at(_pKey); }
	int				GetInt(std::wstring _pKey)		{ return _IntList.at(_pKey); }
	double			GetDouble(std::wstring _pKey)	{ return _DblList.at(_pKey); }
	bool			GetBool(std::wstring _pKey)		{ return _BoolList.at(_pKey); }

	void Clear();
	web::json::value GetJsonValue();

	void Print();

public:
	std::unordered_map<std::wstring, std::wstring>	_StrList;
	std::unordered_map<std::wstring, int>			_IntList;
	std::unordered_map<std::wstring, double>		_DblList;
	std::unordered_map<std::wstring, bool>			_BoolList;
	std::vector<CJsonObject>						_vArrayList;
	std::vector<std::wstring>						_vKeyList;
};




#endif _MATCHINGSERVER_LIB_JSON_H_