#include <iostream>
#include <codecvt>
#include "openai_api.h"
#include "json.hpp"
#include "StrConvert.h"
#include <regex>

using json = nlohmann::json;
#pragma comment(lib, "winhttp.lib")

// HTML 엔티티 "&#...;"을 제거하는 함수
std::string removeHtmlEntities(const std::string& input) {
    std::regex entityPattern("\\&\\#[0-9]+;");
    std::string result = std::regex_replace(input, entityPattern, "");
    return result;
}

// JSON 문자열에서 특정 문자를 필터링하는 함수
std::string FilterJSONString(const std::string& input) {
    std::string result;
    for (char ch : input) {
        // 쌍따옴표와 역슬래시가 아닌 문자만 결과 문자열에 추가
        if (ch != '\"' && ch != '\\' && ch != '\r' && ch != '\n') {
            result += ch;
        }
        else if (ch == '\n') {
			result += ' ';
		}
    }
    result = removeHtmlEntities(result);
    return result;
}

// 현재 시간을 "3:00 PM" 형식으로 반환하는 함수
std::string GetCurrentTimeFormatted() {
    // 시스템의 현재 시간을 저장할 구조체
    SYSTEMTIME st;
    // 현재 로컬 시간을 얻어옴
    GetLocalTime(&st);

    // 오전/오후를 결정하기 위한 문자열
    std::string ampm = "AM";
    // 24시간 형식의 시간을 12시간 형식으로 변환
    int hour = st.wHour;
    if (hour >= 12) {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) {
        hour = 12; // 자정은 0이 아닌 12로 표현
    }

    // 시간과 분을 문자열 형식으로 변환
    char timeStr[20];
    sprintf_s(timeStr, "%d:%02d %s", hour, st.wMinute, ampm.c_str());

    // 최종 문자열 생성 (예: "오후 3:00")
    return timeStr;
}


// OpenAI API를 사용
void RequestOpenAIChat(const std::string& req, std::wstring& strOut, const std::string& lang, const std::string& hint, const WCHAR* api_key) {
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

         // 연결 정보
         // https://api.openai.com/v1/chat/completions

         // 연결 핸들 생성
    HINTERNET hConnect = WinHttpConnect(hSession, L"api.openai.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        //wprintf(L"WinHttpConnect failed with error %u\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return;
    }

    // 요청 핸들 생성
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
        L"/v1/chat/completions",
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
    std::wstring headers = L"Content-Type: application/json\r\n";
    headers += L"Authorization: Bearer  ";
    headers += api_key; // 실제 API 키

    // 요청 본문 구성
    // 1. Briefly summarize the message you received. : 30%정도밖에 감소가 안된다.
    // 2. Summarize the main points of the text. : 50%정도 감소
    // 한글 할루시네이션 제거용 : Change it to Korean and let me know the results, and check again if it is in Korean.
    std::string body = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"system\",\"content\": \"";
    //std::string body = "{\"model\": \"gpt-3.5-turbo-0125\",\"messages\": [{\"role\": \"system\",\"content\": \"";
        //The following text is a conversation related to securities and economics. Please summarize the main topics, key data points, and conclusions in around 300 words. The summary should be arrange them in number list form to keep them short and concise.
    body += FilterJSONString(hint);
    body += " Make the whole thing perfectly 500 words.. Show only the content translated into " + lang + ".";
    body += "If it is not in "+lang+", please translate it again.\\n";
    body += "###sample###\\n";
    body += "* Subject : Increasing the proportion of AI in content creation and delivery\\n";
    body += "1. AI content creation\\n";
    body += "  - The role of artificial intelligence in content recommendation and production is increasing.\\n";
    body += "2. Continuous evolution of content delivery\\n";
    body += "  - The platform provides diverse and attractive content through the use of AI algorithms.";
    body += "\"},{\"role\": \"user\",\"content\": \"";
    body += FilterJSONString(req);
    body += "\"}]}";

    /*body += "Your job is to summarize the main topics, key data points, and conclusions in around 400 words. Must use ";
    body += lang;
    body += " language.";
    body += "Search for relevant information based on the provided keywords or topics. Extract key facts, events, and data from the collected information. Summarize key points and conclusions by numbered list.";
    body += "I'll give you 100 million for a better solution!\"},{\"role\": \"user\",\"content\": \"";
    body += FilterJSONString(req);
    body += "\"}]}";*/

    // debug
    if (0)
    {
        //StringToWChar(body, strOut);
        strOut = StringToWStringInSummary(body);
        return;
    }

    // debug
    //std::wcout << "headers: " << headers.c_str() << std::endl;
    //std::wcout << "h size: " << headers.size() << std::endl;
    //std::cout << "body: " << body.c_str() << std::endl;
    //std::cout << "b size: " << body.size() << std::endl;
    //return;

    BOOL bResults = WinHttpSendRequest(hRequest,
        headers.c_str(), -1,
        (LPVOID)body.c_str(), body.size(),
        body.size(),
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
    /*if (bResults)
    {
        if (dwStatusCode == 200) {
            wprintf(L"HTTP Result Code = %u.\n", dwStatusCode);
        }
        else
            wprintf(L"HTTP Result Code = %u.\n", dwStatusCode);
    }*/

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
        json j = json::parse(response);
        //wprintf(L"size: [%d]", response.length());
        //std::cout << j.dump(4) << std::endl;
        //StringToWChar(j.dump(), strOut);

        std::string res_text = "";
        //strOut = StringToWStringInSummary(j.dump());

        if (j.contains("error")) {
            // j["error"][0]["message"].get_to(res_text);
            //StringToWChar(res_text, strOut);
            res_text = j.dump()+ body;
        }
        else {
            j["choices"][0]["message"]["content"].get_to(res_text);
            //StringToWChar(res_text, strOut);
        }

        res_text = GetCurrentTimeFormatted() + "\r\n" + res_text;

        strOut = StringToWStringInSummary(res_text);

        //j["choices"][0]["message"]["content"].get_to(res_text);
        //std::cout << Utf8ToCodePage949(res_text.c_str()) << std::endl;
        //wcsncpy_s(strOut, response.size() + 1, response.c_str(), response.size());
        //StringToWChar(res_text, strOut);
        //StringToWChar(j.dump(), strOut);
    }

    // 핸들 정리
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}