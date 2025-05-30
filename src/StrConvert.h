﻿#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

int CharToWChar(const char* pstrSrc, wchar_t* pwstrDest); // char 형을 WCHAR* 로 변환하는 함수 : Locale 사용
std::wstring StringToWStringByLocale(const std::string& str); // string을 wstring으로 변환하는 함수 : Locale 사용
std::string wstringToAscii(const std::wstring& wstr); // wstring을 ASCII로 변환하는 함수 : 0x0080 이하의 문자만 추가
std::string WStringToString(const std::wstring& wstr); // wstring을 string으로 변환하는 함수 : Locale 사용
std::wstring StringToWString(const std::string& str); // string을 wstring으로 변환하는 함수 : Locale 사용
std::wstring StringToWCHAR(const std::string& utf8String); // string을 WCHAR*로 변환하는 함수 : MultiByteToWideChar 사용
std::string WCHARToString(const WCHAR* wcharArray); // WCHAR을 std::string (UTF-8)으로 변환하는 함수 : NULL값 제거
std::string WCharToString(const WCHAR* wcharArray); // WCHAR을 std::string (UTF-8)으로 변환하는 함수 : NULL값 포함
int wstringToInt(const std::wstring& wstr); // wstring을 int로 변환하는 함수
bool CompareStringWString(const std::string& str, const std::wstring& wstr); // string과 wstring을 비교하는 함수
std::wstring StringToWStringInSummary(const std::string& str); // string을 wstring으로 변환하는 함수 : MultiByteToWideChar 사용
std::string ConvertToUTF8(const std::string& input); // string을 UTF-8로 변환하는 함수
int UTF8toUTF16(const std::string& utf8, WCHAR* outWStr); // UTF-8을 UTF-16으로 변환하는 함수
int UTF16toUTF8(const std::wstring& utf16, char* outStr); // UTF-16을 UTF-8로 변환하는 함수
std::wstring Utf8ToWideString(const std::string& utf8Str); // UTF-8을 wstring으로 변환하는 함수 -  utf-8 binary 해석용
std::string UTF8ToEucKr(const std::string& utf8_str); // UTF-8을 EUC-KR로 변환하는 함수
std::string ConvertBinaryToString(const std::string& binaryData); // binary를 string으로 변환하는 함수
std::wstring ConvertBinaryToWString(const std::string& binaryData); // binary를 wstring으로 변환하는 함수
std::wstring ConvertToHexString(const std::wstring& input); // std::wstring을 받아 각 wchar_t를 16진수 문자열로 변환하는 함수
std::string Trim(std::string& input); // left, right trim : ' ', '\r', '\n' 제거
std::vector<std::string> CutBySize(std::string txt, int size); // 해당 사이즈가 넘지않는 최대 사이즈로 txt를 잘라서 리턴
