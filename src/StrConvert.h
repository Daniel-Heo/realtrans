#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>

void CharToWChar(wchar_t* pwstrDest, const char* pstrSrc); // char 형을 WCHAR* 로 변환하는 함수 : Locale 사용
std::string Utf8ToCodePage949(const std::string& utf8Str); // UTF-8 문자열을 코드 페이지 949로 변환합니다.
std::string WstrToStr(const std::wstring& source); // wstring을 string으로 변환하는 함수 : 단순변환
std::string wstringToAscii(const std::wstring& wstr); // wstring을 ASCII로 변환하는 함수 : 0x0080 이하의 문자만 추가
std::string WStringToString(const std::wstring& wstr); // wstring을 string으로 변환하는 함수 : Locale 사용
std::wstring StringToWString(const std::string& str); // string을 wstring으로 변환하는 함수 : Locale 사용
void StringToWChar(const std::string& s, WCHAR* outStr); // string을 WCHAR*로 변환하는 함수
std::wstring EucKrToWString(const std::string& eucKrStr); // EUC-KR 문자열을 UTF-16으로 변환합니다.
std::wstring StringToWCHAR(const std::string& utf8String); // string을 WCHAR*로 변환하는 함수 : MultiByteToWideChar 사용
//std::string WCHARToString2(const WCHAR* wcharArray); // WCHAR을 std::string (UTF-8)으로 변환하는 함수
std::string WCHARToString(const WCHAR* wcharArray); // WCHAR을 std::string (UTF-8)으로 변환하는 함수 : NULL값 제거
int wstringToInt(const std::wstring& wstr); // wstring을 int로 변환하는 함수
