#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>

int CharToWChar(wchar_t* pwstrDest, const char* pstrSrc); // char ���� WCHAR* �� ��ȯ�ϴ� �Լ� : Locale ���
std::string WstrToStr(const std::wstring& source); // wstring�� string���� ��ȯ�ϴ� �Լ� : �ܼ���ȯ
std::string wstringToAscii(const std::wstring& wstr); // wstring�� ASCII�� ��ȯ�ϴ� �Լ� : 0x0080 ������ ���ڸ� �߰�
std::string WStringToString(const std::wstring& wstr); // wstring�� string���� ��ȯ�ϴ� �Լ� : Locale ���
std::wstring StringToWString(const std::string& str); // string�� wstring���� ��ȯ�ϴ� �Լ� : Locale ���
std::wstring StringToWCHAR(const std::string& utf8String); // string�� WCHAR*�� ��ȯ�ϴ� �Լ� : MultiByteToWideChar ���
std::string WCHARToString(const WCHAR* wcharArray); // WCHAR�� std::string (UTF-8)���� ��ȯ�ϴ� �Լ� : NULL�� ����
int wstringToInt(const std::wstring& wstr); // wstring�� int�� ��ȯ�ϴ� �Լ�
BOOL CompareStringAndWString(const std::string& str, const std::wstring& wstr); // string�� wstring�� ���ϴ� �Լ�
std::wstring StringToWStringInSummary(const std::string& str); // string�� wstring���� ��ȯ�ϴ� �Լ� : MultiByteToWideChar ���
std::string ConvertToUTF8(const std::string& input); // string�� UTF-8�� ��ȯ�ϴ� �Լ�
int UTF8toUTF16(const std::string& utf8, WCHAR* outWStr); // UTF-8�� UTF-16���� ��ȯ�ϴ� �Լ�
int UTF16toUTF8(const std::wstring& utf16, char* outStr); // UTF-16�� UTF-8�� ��ȯ�ϴ� �Լ�
std::wstring Utf8ToWideString(const std::string& utf8Str); // UTF-8�� wstring���� ��ȯ�ϴ� �Լ� -  utf-8 binary �ؼ���
std::string ConvertBinaryToString(const std::string& binaryData); // binary�� string���� ��ȯ�ϴ� �Լ�
std::wstring ConvertBinaryToWString(const std::string& binaryData); // binary�� wstring���� ��ȯ�ϴ� �Լ�
