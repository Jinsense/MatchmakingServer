#include "HttpURL.h"
/*
bool Http()
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;

	size_t WriteMemoryCallback(char *ptr, size_t size, size_t nmemb, void * userdata);

	struct curl_slist *headerlist = nullptr;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json");

	strTargetURL = "https://www.example.com/bind";
	strResourceJSON = "{\"snippet\": {\"title\": \"this is title\", \"scheduledStartTime\": \"2017-05-15\"},\"status\": {\"privacyStatus\": \"private\"}}";

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

		res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}

		std::cout << "------------Result" << std::endl;
		std::cout << chunk.memory << std::endl;

		return true;
	}
	return false;
}

LPBYTE PostWebPage(LPCTSTR pszURL, LPBYTE pbPostData)
{
	int     result;
	DWORD    dwLength;
	int     nContentLength;
	BYTE    baBuffer[64];
	DWORD    dwServiceType;
	INTERNET_PORT  nPort;
	HINTERNET   hInternet = NULL;
	HINTERNET   hConnect = NULL;
	HINTERNET   hRequest = NULL;
	CString    sServer, sObject;
	CString    strHeaders = _T("Content-Type: application/x-www-form-urlencoded");
	LPBYTE    pbContent = NULL;
	CString    sLicenseXML;
	LPSTR    pszFirst = NULL, pszLast = NULL;
	CString sLog;
	DWORD dwFlags = 0;
	CString ss;
	//
	// URL 호출
	//
	hInternet = InternetOpen(_T("Mozilla"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		result = GetLastError();
		goto finish;
	}
	result = AfxParseURL(pszURL, dwServiceType, sServer, sObject, nPort);
	if (result == FALSE)
	{
		result = GetLastError();
		goto finish;
	}


	hConnect = InternetConnect(hInternet, sServer, nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if (hConnect == NULL)
	{
		result = GetLastError();
		goto finish;
	}

	//
	// 반드시 포스트 방식으로
	//
	hRequest = HttpOpenRequest(hConnect, _T("POST"), sObject, NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, NULL);
	if (hRequest == NULL)
	{
		result = GetLastError();
		goto finish;
	}

	//
	// 웹서버에 데이타를 전송
	//
	result = HttpSendRequest(hRequest, strHeaders, strHeaders.GetLength(), (LPVOID)pbPostData, strlen((LPSTR)pbPostData));
	if (result == FALSE)
	{
		result = GetLastError();

		goto finish;
	}

	//
	// HTTP STATUS 코드
	//
	dwLength = sizeof(baBuffer);
	result = HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, (LPVOID)baBuffer, &dwLength, NULL);
	if (result == FALSE)
	{
		result = GetLastError();
		goto finish;
	}

	if (strcmp((LPCSTR)baBuffer, "200") != 0)
	{
		//
		// HTTP Request 가 실패했다. 왜???
		//
		//CString  s = (LPCSTR)baBuffer;
		result = ERROR_HTTP_INVALID_QUERY_REQUEST;
		goto finish;
	}

	dwLength = sizeof(baBuffer);
	result = HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_LENGTH, (LPVOID)baBuffer, &dwLength, NULL);
	if (result == FALSE)
	{
		result = GetLastError();
		goto finish;
	}

	nContentLength = atoi((LPSTR)baBuffer);
	pbContent = new BYTE[nContentLength + 1];
	if (pbContent == NULL)
	{
		result = ERROR_NOT_ENOUGH_MEMORY;
		goto finish;
	}

	result = InternetReadFile(hRequest, pbContent, nContentLength, &dwLength);
	if (result == FALSE)
	{
		result = GetLastError();
		goto finish;
	}

	pbContent[nContentLength] = '\0';
	result = 0;
finish:
	if (hInternet != NULL)
		InternetCloseHandle(hInternet);
	if (hConnect != NULL)
		InternetCloseHandle(hConnect);
	if (hRequest != NULL)
		InternetCloseHandle(hRequest);

	return pbContent;
}
*/