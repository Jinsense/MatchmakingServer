#ifndef _MATCHINGSERVER_LIB_HTTP_H_
#define _MATCHINGSERVER_LIB_HTTP_H_

#define CURL_STATICLIB

#include <iostream>
#include <string>
#include <cpprest\http_client.h>		//	restsdk 사용할 경우
#include <cpprest\filestream.h>			//	restsdk 사용할 경우

//	#include "../curl/include/curl.h"		//	CURL lib 사용할 경우

#pragma comment(lib, "cpprest120_2_4")	// Windows Only
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace std;

char * HttpRequest(char * pJson)
{
	json::value postData;
	postData[L"name"] = json::value::string(L"Joe Smith");
	postData[L"sport"] = json::value::string(L"Baseball");

	http_client client(L"http://localhost:5540/api/values");

	client.request(methods::POST, L"", postData.to_string().c_str(),
		L"application/json").then([](http_response response)
	{
		std::wcout << response.status_code() << std::endl;

		if (response.status_code() == status_codes::OK)
		{
			auto body = response.extract_string();

			std::wcout << body.get().c_str();
		}
	});

	return pJson;
}



#endif _MATCHINGSERVER_LIB_HTTP_H_