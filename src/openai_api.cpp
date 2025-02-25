#include <iostream>
#include <codecvt>
#include "openai_api.h"
#include "json.hpp"
#include "StrConvert.h"
#include <regex>

using json = nlohmann::json;
#pragma comment(lib, "winhttp.lib")

// HTML ��ƼƼ "&#...;"�� �����ϴ� �Լ�
std::string removeHtmlEntities(const std::string& input) {
    std::regex entityPattern("\\&\\#[0-9]+;");
    std::string result = std::regex_replace(input, entityPattern, "");
    return result;
}

// JSON ���ڿ����� Ư�� ���ڸ� ���͸��ϴ� �Լ�
std::string FilterJSONString(const std::string& input) {
    std::string result;
    for (char ch : input) {
        // �ֵ���ǥ�� �������ð� �ƴ� ���ڸ� ��� ���ڿ��� �߰�
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

// ���� �ð��� "3:00 PM" �������� ��ȯ�ϴ� �Լ�
std::string GetCurrentTimeFormatted() {
    // �ý����� ���� �ð��� ������ ����ü
    SYSTEMTIME st;
    // ���� ���� �ð��� ����
    GetLocalTime(&st);

    // ����/���ĸ� �����ϱ� ���� ���ڿ�
    std::string ampm = "AM";
    // 24�ð� ������ �ð��� 12�ð� �������� ��ȯ
    int hour = st.wHour;
    if (hour >= 12) {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) {
        hour = 12; // ������ 0�� �ƴ� 12�� ǥ��
    }

    // �ð��� ���� ���ڿ� �������� ��ȯ
    char timeStr[20];
    sprintf_s(timeStr, "%d:%02d %s", hour, st.wMinute, ampm.c_str());

    // ���� ���ڿ� ���� (��: "���� 3:00")
    return timeStr;
}


// OpenAI API�� ���
void RequestOpenAIChat(const std::string& req, std::wstring& strOut, const std::string& lang, const std::string& hint, const WCHAR* api_key) {
    // WinHTTP ���� �ʱ�ȭ
    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) {
        //wprintf(L"WinHttpOpen failed with error %u\n", GetLastError());
        return;
    }

         // ���� ����
         // https://api.openai.com/v1/chat/completions

         // ���� �ڵ� ����
    HINTERNET hConnect = WinHttpConnect(hSession, L"api.openai.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        //wprintf(L"WinHttpConnect failed with error %u\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return;
    }

    // ��û �ڵ� ����
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

    // �ʿ��� ��� �߰�
    std::wstring headers = L"Content-Type: application/json\r\n";
    headers += L"Authorization: Bearer  ";
    headers += api_key; // ���� API Ű

    // ��û ���� ����
    // 1. Briefly summarize the message you received. : 30%�����ۿ� ���Ұ� �ȵȴ�.
    // 2. Summarize the main points of the text. : 50%���� ����
    // �ѱ� �ҷ�ó��̼� ���ſ� : Change it to Korean and let me know the results, and check again if it is in Korean.
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

    // ���� �ޱ�
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

    // ���� ���� �б�
    //DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    std::string response = "";
    if (bResults) {
        do {
            // ���� ũ�� Ȯ��
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                //wprintf(L"Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
            }

            // ���� ���� �Ҵ� �� �б�
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

        // ���� Ȯ��
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

    // �ڵ� ����
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}