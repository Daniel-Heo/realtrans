#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>

void CharToWChar(wchar_t* pwstrDest, const char* pstrSrc); // char ���� WCHAR* �� ��ȯ�ϴ� �Լ� : Locale ���
std::string Utf8ToCodePage949(const std::string& utf8Str); // UTF-8 ���ڿ��� �ڵ� ������ 949�� ��ȯ�մϴ�.
std::string WstrToStr(const std::wstring& source); // wstring�� string���� ��ȯ�ϴ� �Լ� : �ܼ���ȯ
std::string wstringToAscii(const std::wstring& wstr); // wstring�� ASCII�� ��ȯ�ϴ� �Լ� : 0x0080 ������ ���ڸ� �߰�
std::string WStringToString(const std::wstring& wstr); // wstring�� string���� ��ȯ�ϴ� �Լ� : Locale ���
std::wstring StringToWString(const std::string& str); // string�� wstring���� ��ȯ�ϴ� �Լ� : Locale ���
void StringToWChar(const std::string& s, WCHAR* outStr); // string�� WCHAR*�� ��ȯ�ϴ� �Լ�
std::wstring EucKrToWString(const std::string& eucKrStr); // EUC-KR ���ڿ��� UTF-16���� ��ȯ�մϴ�.
std::wstring StringToWCHAR(const std::string& utf8String); // string�� WCHAR*�� ��ȯ�ϴ� �Լ� : MultiByteToWideChar ���
//std::string WCHARToString2(const WCHAR* wcharArray); // WCHAR�� std::string (UTF-8)���� ��ȯ�ϴ� �Լ�
std::string WCHARToString(const WCHAR* wcharArray); // WCHAR�� std::string (UTF-8)���� ��ȯ�ϴ� �Լ� : NULL�� ����
int wstringToInt(const std::wstring& wstr); // wstring�� int�� ��ȯ�ϴ� �Լ�
