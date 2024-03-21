// 요약해주는 richedit dialogue control class
#include <iostream>
#include <codecvt>
#include "StrConvert.h"
#include "json.hpp" // 또는 경로를 지정해야 할 경우: #include "External/json.hpp"
#include "CDlgSummary.h"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;
extern CDlgSummary* DlgSum;

int CharToWChar(wchar_t* pwstrDest, const char* pstrSrc)
{
    size_t cn;
    setlocale(LC_ALL, "");

    // 입력 문자열의 길이를 계산합니다. 반환되는 길이에는 널 종료 문자가 포함되지 않습니다.
    int nLen = MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, NULL, 0);

    // 실제 변환을 수행합니다. CP_ACP (ANSI Code Page)-locale에 기반한 그 지역의 문자 인코딩방식, CP_UTF8 (UTF-8 Code Page)- 국제 호환
    MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, pwstrDest, nLen);
    return nLen;
}

int UTF8toUTF16(const std::string& utf8, WCHAR *outWStr)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, outWStr, len);
        outWStr[len] = 0;
    }
    return len;
}

int UTF16toUTF8(const std::wstring& utf16, char *outStr)
{
    int len = WideCharToMultiByte(CP_ACP, 0, &utf16[0], -1, NULL, 0, 0, 0);
    if (len > 1)
    {
        WideCharToMultiByte(CP_ACP, 0, &utf16[0], -1, outStr, len, 0, 0);
        outStr[len] = 0;
    }
    return len;
}

std::wstring Utf8ToWideString(const std::string& utf8Str) {
    // UTF-8 문자열의 길이를 계산합니다. -1은 널 종료 문자를 포함하도록 합니다.
    int wideCharCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);

    // 충분한 크기의 wstring 버퍼를 준비합니다.
    std::wstring wideStr(wideCharCount, 0);

    // UTF-8 문자열을 UTF-16으로 변환합니다.
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharCount);

    return wideStr;
}

// wstring을 string으로 변환하는 함수 : 단순변환
std::string WstrToStr(const std::wstring& source)
{
    return std::string().assign(source.begin(), source.end());
}

// wstring을 ASCII로 변환하는 함수 : 0x0080 이하의 문자만 추가
std::string wstringToAscii(const std::wstring& wstr) {
    std::string asciiStr;
    for (wchar_t wc : wstr) {
        if (wc < 0x0080) { // ASCII 범위 내의 문자만 추가
            asciiStr += static_cast<char>(wc);
        }
        else {
            // 비ASCII 문자를 대체 문자나 무시
            // 예: asciiStr += '?'; 또는 그냥 무시
        }
    }
    return asciiStr;
}

// string 형을 wstring으로 변환하는 함수 : codecvt 사용
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// string 형을 wstring으로 변환하는 함수 : MultiByteToWideChar 사용
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();

    int charsNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (charsNeeded == 0) {
        throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
    }

    std::vector<wchar_t> buffer(charsNeeded);
    int charsConverted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], charsNeeded);

    if (charsConverted == 0) {
        throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
    }

    return std::wstring(buffer.begin(), buffer.end() - 1); // Remove the null terminator
}

// std::string (UTF-8)을 std::wstring (WCHAR 배열)으로 변환하는 함수
// string 형을 wstring으로 변환하는 함수 : MultiByteToWideChar 사용
std::wstring StringToWStringInSummary(const std::string& str) {
    if (str.empty()) return std::wstring();

    int charsNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (charsNeeded == 0) {
        //throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
        return std::wstring();
    }

    std::vector<wchar_t> buffer(charsNeeded);
    int charsConverted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], charsNeeded);

    if (charsConverted == 0) {
        //throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
        return std::wstring();
    }

    return std::wstring(buffer.begin(), buffer.end() - 1); // Remove the null terminator
}

// std::string (UTF-8)을 std::wstring (WCHAR 배열)으로 변환하는 함수
std::wstring StringToWCHAR(const std::string& utf8String) {
    if (utf8String.empty()) return std::wstring();

    // UTF-8 문자열을 WCHAR 배열로 변환하기 위해 필요한 크기 계산
    int wcharCount = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, NULL, 0);
    if (wcharCount == 0) {
        throw std::runtime_error("Failed to convert UTF-8 string to WCHAR.");
    }

    // WCHAR 배열을 위한 공간 할당
    std::vector<WCHAR> wcharArray(wcharCount);

    // UTF-8 문자열을 WCHAR 배열로 변환
    if (MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &wcharArray[0], wcharCount) == 0) {
        throw std::runtime_error("Failed to convert UTF-8 string to WCHAR.");
    }

    // std::wstring으로 변환하여 반환
    return std::wstring(wcharArray.begin(), wcharArray.end() - 1); // null 종료 문자 제거
}

// WCHAR을 std::string (UTF-8)으로 변환하는 함수 : NULL값 제거
std::string WCHARToString(const WCHAR* wcharArray) {
    if (!wcharArray) return std::string();

    // 필요한 문자열 크기를 계산합니다 (널 종료 문자 포함).
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string(); // 변환에 실패하거나 빈 문자열인 경우

    // sizeNeeded에는 널 종료 문자도 포함되어 있으므로, 이를 제외한 크기로 문자열을 생성합니다.
    std::string strTo(sizeNeeded - 1, 0); // 널 종료 문자를 제외한 크기
    WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, &strTo[0], sizeNeeded, NULL, NULL);

    return strTo;
}


int wstringToInt(const std::wstring& wstr) {
    int number = 0;
    try {
        // std::stoi를 사용하여 std::wstring을 int로 변환
        number = std::stoi(wstr);
    }
    catch (const std::invalid_argument& e) {
        // 변환할 수 없는 경우
        std::wcerr << L"Invalid argument: " << e.what() << std::endl;
        // 적절한 에러 처리
    }
    catch (const std::out_of_range& e) {
        // 변환된 값이 int 범위를 벗어나는 경우
        std::wcerr << L"Out of range: " << e.what() << std::endl;
        // 적절한 에러 처리
    }
    return number;
}

// string과 wstring을 비교
BOOL CompareStringAndWString(const std::string& str, const std::wstring& wstr) {
    // wstring을 string으로 변환
    std::string convertedStr = WStringToString(wstr);
    // 변환된 string과 원본 string을 비교
    return str == convertedStr;
}

void trace(const std::string& msg) {
    OutputDebugStringA(msg.c_str());
}

// 시스템의 현재 코드 페이지에서 UTF-8 문자열로 변환
std::string ConvertToUTF8(const std::string& input) {
    // 먼저, 입력 문자열을 UTF-16으로 변환
    int wcharCount = MultiByteToWideChar(GetConsoleCP(), 0, input.c_str(), -1, NULL, 0);
    std::vector<WCHAR> wstr(wcharCount);
    MultiByteToWideChar(GetConsoleCP(), 0, input.c_str(), -1, &wstr[0], wcharCount);

    // 그 다음, UTF-16 문자열을 UTF-8로 변환
    int utf8Count = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
    std::vector<char> utf8Str(utf8Count);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &utf8Str[0], utf8Count, NULL, NULL);

    return std::string(utf8Str.begin(), utf8Str.end() - 1); // null 종료 문자 제거
}

std::string ConvertBinaryToString(const std::string& binaryData) {
    // 바이너리 데이터에서 b' '로 묶인 부분을 찾아냅니다.
    size_t start = binaryData.find("b'");
    size_t end = binaryData.rfind("'");

    if (start == std::string::npos) {
        start = binaryData.find("b\"");
        end = binaryData.rfind("\"");
	}

    if (start != std::string::npos && end != std::string::npos && start < end) {
        // b' '로 묶인 내용을 추출합니다.
        std::string trimmed = binaryData.substr(start + 2, end - start - 2);

        // 추출된 문자열에서 백슬래시 이스케이프 시퀀스를 실제 바이트 값으로 변환합니다.
        std::string result;
        for (size_t i = 0; i < trimmed.length(); ++i) {
            // 문자 filter ( \n, \t, \r, \xHH )
            if (trimmed[i] == '\n' || trimmed[i]=='\r') {
                i++;
                continue;
            }
            if (trimmed[i] == '\\' && i + 3 < trimmed.length() && trimmed[i + 1] == 'x') {
                // \xHH 형식의 16진수 값을 문자로 변환합니다.
                std::string hexValue = trimmed.substr(i + 2, 2);
                char byteValue = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                result.push_back(byteValue);
                i += 3; // 16진수 값과 이스케이프 시퀀스를 건너뜁니다.
            }
            else {
                result.push_back(trimmed[i]);
            }
        }
        return result;
    }

    // b' ' 형식이 아니라면 원본 데이터를 그대로 반환합니다.
    return binaryData;
}

std::wstring ConvertBinaryToWString(const std::string& binaryData) {
    // 바이너리 데이터에서 b' '로 묶인 부분을 제거합니다.
    size_t start = binaryData.find("b'");
    size_t end = binaryData.rfind("'");
    std::string trimmed;

    if (start != std::string::npos && end != std::string::npos && start < end) {
        trimmed = binaryData.substr(start + 2, end - start - 2);
    }
    else {
        // b' ' 형식이 아니면 원본 데이터를 사용합니다.
        trimmed = binaryData;
    }

    // UTF-8 문자열을 UTF-16으로 변환합니다.
    int wlen = MultiByteToWideChar(CP_UTF8, 0, trimmed.c_str(), -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, trimmed.c_str(), -1, &wstr[0], wlen);

    return wstr;
}