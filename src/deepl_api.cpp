#include <iostream>
#include <codecvt>
#include "openai_api.h"
#include "json.hpp"
#include "StrConvert.h"
//#include "CDlgSummary.h"

using json = nlohmann::json;
#pragma comment(lib, "winhttp.lib")

//extern CDlgSummary* DlgSum;

// 입력 문자열에서 특정 문자를 필터링하는 함수
std::string FilterString(const std::string& input) {
    std::string result;
    for (char ch : input) {
        if (ch != '\"' && ch != '\\' && ch != '\r' && ch != '\n') {
            result += ch;
        }
        else if (ch == '\n') {
            result += ' ';
        }
    }
    return result;
}

// DeepL API를 사용하여 번역할 경우
//  결과는 가져오는데 내용이 없는 경우가 있다. 이것은 deepL에서 발생하는 문제로 보인다.
void RequestDeepL(const std::string& text, std::string& strOut, const std::string& lang, const std::string& api_key) {
    // WinHTTP 세션 초기화
    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) {
        //wprintf(L"WinHttpOpen failed with error %u\n", GetLastError());
        return;
    }

    // HTTP/2 사용 설정
    DWORD dwHttp2Option = WINHTTP_PROTOCOL_FLAG_HTTP2;
    BOOL bHttp2Enabled = WinHttpSetOption(hSession,
        WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
        &dwHttp2Option,
        sizeof(dwHttp2Option));

    //if (!bHttp2Enabled) {
    //    // HTTP/2 설정 실패 처리
    //    std::cout << "Failed to enable HTTP/2: " << GetLastError() << std::endl;
    //}
    //else {
    //    // HTTP/2 설정 성공 처리
    //    std::cout << "HTTP/2 enabled successfully" << std::endl;
    //}

    // 연결 핸들 생성
    HINTERNET hConnect = WinHttpConnect(hSession, L"api-free.deepl.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        //wprintf(L"WinHttpConnect failed with error %u\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return;
    }

    // 요청 핸들 생성
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
        L"/v2/translate",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        //wprintf(L"WinHttpOpenRequest failed with error %u\n", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    // 필요한 헤더 추가
    //std::wstring headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    std::wstring headers = L"Content-Type: application/json\r\n";
    headers += L"Authorization: DeepL-Auth-Key ";
    headers += StringToWString(api_key);
    //std::string postData = "text=" + FilterString(text) + "&target_lang=" + lang;
    std::string postData = "{\"target_lang\": \"";
    postData += lang;
    postData += "\",\"text\": [\"" + FilterString(text) + "\"]}";

    //DlgSum->Trace(StringToWString(postData));

    // debug
    if (0)
    {
        //std::wcout << "headers: " << headers.c_str() << std::endl;
        //trace("RequestDeepL: " + postData);
        return;
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
        headers.c_str(), -1,
        (LPVOID)postData.c_str(), postData.size(),
        postData.size(),
        0);

    // 응답 받기
    if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);
    else {
        //wprintf(L"Error %u in WinHttpSendRequest.\n", GetLastError());
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);
        return;
    }

    // Use WinHttpQueryHeaders to obtain the header buffer.
    DWORD dwSize = sizeof(DWORD);
    DWORD dwStatusCode = 0;
    if (bResults)
        bResults = WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL,
            &dwStatusCode,
            &dwSize,
            WINHTTP_NO_HEADER_INDEX);

    // Based on the status code, determine whether 
    // the document was recently updated.
    /*
    if (bResults)
    {
        if (dwStatusCode == 200) {
            printf("HTTP Result Code = %u.\n", dwStatusCode);
        }
        else
            printf("HTTP Result Code = %u.\n", dwStatusCode);
    }
    */

    // 응답 내용 읽기
    //DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    std::string response = "";
    if (bResults) {
        do {
            // 응답 크기 확인
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                //wprintf(L"Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
            }

            // 응답 내용 할당 및 읽기
            pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer) {
                //wprintf(L"Out of memory\n");
                dwSize = 0;
            }
            else {
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
                    dwSize, &dwDownloaded)) {
                    //wprintf(L"Error %u in WinHttpReadData.\n", GetLastError());
                }
                else {
                    response += pszOutBuffer;
                }
                delete[] pszOutBuffer;
            }
        } while (dwSize > 0);

        // 응답 확인
        //json j = json::parse(response);
        //wprintf(L"size: [%d]", response.length());
        //std::cout << j.dump(4) << std::endl;
        //StringToWChar(j.dump(), strOut);

        //trace("RequestDeepL: "+response);
        //std::cout << "Response: " << response << std::endl;

        json j = json::parse(response);
        //std::string res_text = "";
        if (dwStatusCode == 200)
        {
            if (j.contains("translations")) {
                //{"translations": [{"detected_source_language":"EN", "text" : "결과"}] }
                j["translations"][0]["text"].get_to(strOut);
			}
            else {
                // {"message":"Value for 'target_lang' not supported."}
                j["message"].get_to(strOut);
			}
		}
        else
        {
            if (j.contains("message"))
                j["message"].get_to(strOut);
            else strOut="Unknown error code:"+ std::to_string(dwStatusCode);
		}

        //DlgSum->Trace(StringToWString(j.dump()));
        //strOut = std::to_string(dwStatusCode) + ":" + strOut;
        //std::cout << Utf8ToCodePage949(strOut.c_str()) << std::endl;
    }

    // 핸들 정리
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}

/*
int main()
{
    // DeepL API
    std::string apiKey = "xx";
    std::string textToTranslate = "Hello, world!";
    std::string targetLanguage = "KO"; // 한국어로 번역
    std::string strOut;
    RequestDeepL(textToTranslate, strOut, targetLanguage, apiKey);
}
*/