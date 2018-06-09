#include "Json.h"

CJsonObject::CJsonObject()
{

}

CJsonObject::CJsonObject(web::json::value _pValue, std::vector<std::wstring> _pKeyList)
{
	if (!_pKeyList.empty())
		_vKeyList = _pKeyList;

	if (_pValue.is_array())
	{
		web::json::array A = _pValue.as_array();
		for (auto v : A)
			_vArrayList.emplace_back(v, _vKeyList);
	}
	else if (_pValue.is_object())
	{
		std::vector<web::json::value> V;

		for (auto sKey : _vKeyList)
		{
			web::json::value v = _pValue.at(sKey);
			if (v.is_string())
				_StrList.emplace(sKey, v.as_string());
			else if (v.is_integer())
				_IntList.emplace(sKey, v.as_integer());
			else if (v.is_double())
				_DblList.emplace(sKey, v.as_double());
			else if (v.is_boolean())
				_BoolList.emplace(sKey, v.as_bool());
		}
	}
}

void CJsonObject::Clear()
{
	_StrList.clear();
	_IntList.clear();
	_DblList.clear();
	_BoolList.clear();
	_vArrayList.clear();

	return;
}

web::json::value CJsonObject::GetJsonValue()
{
	web::json::value V;
	if (isArray())
	{
		std::vector<web::json::value> VJ;
		for (auto JO : _vArrayList)
			VJ.emplace_back(JO.GetJsonValue());
		V = web::json::value::array(VJ);
	}
	else
	{
		for (auto PS : _StrList)
			V[PS.first] = web::json::value::string(PS.second);
		for (auto PI : _IntList)
			V[PI.first] = web::json::value::number(PI.second);
		for (auto PD : _DblList)
			V[PD.first] = web::json::value::number(PD.second);
		for (auto PB : _BoolList)
			V[PB.first] = web::json::value::boolean(PB.second);
	}
	return V;
}

void CJsonObject::Print()
{
	wprintf(U("%s"), GetJsonValue().serialize().c_str());
}