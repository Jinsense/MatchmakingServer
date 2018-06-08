#ifndef _MATCHINGSERVER_LIB_JSON_H_
#define _MATCHINGSERVER_LIB_JSON_H_

#include "../json/include/rapidjson/document.h"
#include "../json/include/rapidjson/writer.h"
#include "../json/include/rapidjson/stringbuffer.h"

using namespace rapidjson;

StringBuffer StringJSON;
Writer<StringBuffer, UTF16<>> writer(StringJSON);




#endif