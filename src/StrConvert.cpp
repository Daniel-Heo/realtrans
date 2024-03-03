// 요약해주는 richedit dialogue control class
#include <iostream>
#include <codecvt>
#include "StrConvert.h"
#include "json.hpp" // 또는 경로를 지정해야 할 경우: #include "External/json.hpp"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

// char 형을 WCHAR* 로 변환하는 함수 : Locale 사용
void CharToWChar(wchar_t* pwstrDest, const char* pstrSrc)
{
    // 1. mbstowcs_s 함수를 사용
    size_t cn;
    setlocale(LC_ALL, "ko-KR");
    // 먼저 변환될 와이드 문자의 개수를 얻기 위해 mbstowcs_s 함수를 호출합니다.
    mbstowcs_s(&cn, nullptr, 0, pstrSrc, _TRUNCATE);
    mbstowcs_s(&cn, pwstrDest, cn + 1, pstrSrc, _TRUNCATE);

    // 2. MultiByteToWideChar 함수를 사용
    // 입력 문자열의 길이를 계산합니다. 반환되는 길이에는 널 종료 문자가 포함되지 않습니다.
    //int nLen = MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, NULL, 0);

    // 실제 변환을 수행합니다. CP_ACP (ANSI Code Page)-locale에 기반한 그 지역의 문자 인코딩방식, CP_UTF8 (UTF-8 Code Page)- 국제 호환
    //MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, pwstrDest, nLen);
}

// UTF-8 문자열을 코드 페이지 949로 변환합니다.
std::string Utf8ToCodePage949(const std::string& utf8Str) {
    // UTF-8을 UTF-16(유니코드)로 변환
    int wideCharsCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideCharsCount == 0) {
        throw std::runtime_error("Failed converting UTF-8 to UTF-16.");
    }

    std::vector<wchar_t> wideChars(wideCharsCount);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideChars[0], wideCharsCount);

    // UTF-16을 코드 페이지 949로 변환
    int multiByteCount = WideCharToMultiByte(949, 0, &wideChars[0], -1, NULL, 0, NULL, NULL);
    if (multiByteCount == 0) {
        throw std::runtime_error("Failed converting UTF-16 to Code Page 949.");
    }

    std::vector<char> multiBytes(multiByteCount);
    WideCharToMultiByte(949, 0, &wideChars[0], -1, &multiBytes[0], multiByteCount, NULL, NULL);

    return std::string(multiBytes.begin(), multiBytes.end() - 1); // 마지막 널 문자 제거
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

// string 형을 WCHAR* 로 변환하는 함수
void StringToWChar(const std::string& s, WCHAR* outStr) {
    // 변환된 문자열의 길이를 계산합니다. 멀티바이트를 와이드 문자로 변환할 때 필요한 길이를 얻습니다.
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);

    // 실제 변환을 수행합니다.
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, outStr, len);
}

// EUC-KR 문자열을 UTF-16으로 변환합니다.
std::wstring EucKrToWString(const std::string& eucKrStr) {
    if (eucKrStr.empty()) return std::wstring();

    // EUC-KR을 유니코드로 변환할 때 사용하는 코드 페이지 번호는 949입니다.
    const UINT codePage = 949;

    // 필요한 와이드 문자의 수를 계산합니다.
    int charsNeeded = MultiByteToWideChar(codePage, 0, eucKrStr.c_str(), -1, NULL, 0);
    if (charsNeeded == 0) {
        throw std::runtime_error("Failed converting EUC-KR string to UTF-16");
    }

    std::vector<wchar_t> buffer(charsNeeded);
    int charsConverted = MultiByteToWideChar(codePage, 0, eucKrStr.c_str(), -1, buffer.data(), charsNeeded);

    if (charsConverted == 0) {
        throw std::runtime_error("Failed converting EUC-KR string to UTF-16");
    }

    // 버퍼에서 널 종료 문자를 제외하고 std::wstring 객체를 생성합니다.
    return std::wstring(buffer.begin(), buffer.end() - 1);
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

// WCHAR을 std::string (UTF-8)으로 변환하는 함수
std::string WCHARToString2(const WCHAR* wcharArray) {
    if (!wcharArray) return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, &strTo[0], sizeNeeded, NULL, NULL);

    return strTo;
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

void trace(const std::string& msg) {
    OutputDebugStringA(msg.c_str());
}