#ifndef _MATCHINGSERVER_LIB_HTTP_H_
#define _MATCHINGSERVER_LIB_HTTP_H_

//	#define CURL_STATICLIB

#include <cpprest\http_client.h>
#include <cpprest\filestream.h>

//	#include "../curl/include/curl.h"		//	CURL lib 이용할 경우

#pragma comment(lib, "cpprest120_2_4")		// Windows Only
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams












#endif

