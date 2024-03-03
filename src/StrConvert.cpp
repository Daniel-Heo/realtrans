// ������ִ� richedit dialogue control class
#include <iostream>
#include <codecvt>
#include "StrConvert.h"
#include "json.hpp" // �Ǵ� ��θ� �����ؾ� �� ���: #include "External/json.hpp"

// nlohmann ���̺귯���� ���ӽ����̽� ���
using json = nlohmann::json;

// char ���� WCHAR* �� ��ȯ�ϴ� �Լ� : Locale ���
void CharToWChar(wchar_t* pwstrDest, const char* pstrSrc)
{
    // 1. mbstowcs_s �Լ��� ���
    size_t cn;
    setlocale(LC_ALL, "ko-KR");
    // ���� ��ȯ�� ���̵� ������ ������ ��� ���� mbstowcs_s �Լ��� ȣ���մϴ�.
    mbstowcs_s(&cn, nullptr, 0, pstrSrc, _TRUNCATE);
    mbstowcs_s(&cn, pwstrDest, cn + 1, pstrSrc, _TRUNCATE);

    // 2. MultiByteToWideChar �Լ��� ���
    // �Է� ���ڿ��� ���̸� ����մϴ�. ��ȯ�Ǵ� ���̿��� �� ���� ���ڰ� ���Ե��� �ʽ��ϴ�.
    //int nLen = MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, NULL, 0);

    // ���� ��ȯ�� �����մϴ�. CP_ACP (ANSI Code Page)-locale�� ����� �� ������ ���� ���ڵ����, CP_UTF8 (UTF-8 Code Page)- ���� ȣȯ
    //MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, pwstrDest, nLen);
}

// UTF-8 ���ڿ��� �ڵ� ������ 949�� ��ȯ�մϴ�.
std::string Utf8ToCodePage949(const std::string& utf8Str) {
    // UTF-8�� UTF-16(�����ڵ�)�� ��ȯ
    int wideCharsCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideCharsCount == 0) {
        throw std::runtime_error("Failed converting UTF-8 to UTF-16.");
    }

    std::vector<wchar_t> wideChars(wideCharsCount);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideChars[0], wideCharsCount);

    // UTF-16�� �ڵ� ������ 949�� ��ȯ
    int multiByteCount = WideCharToMultiByte(949, 0, &wideChars[0], -1, NULL, 0, NULL, NULL);
    if (multiByteCount == 0) {
        throw std::runtime_error("Failed converting UTF-16 to Code Page 949.");
    }

    std::vector<char> multiBytes(multiByteCount);
    WideCharToMultiByte(949, 0, &wideChars[0], -1, &multiBytes[0], multiByteCount, NULL, NULL);

    return std::string(multiBytes.begin(), multiBytes.end() - 1); // ������ �� ���� ����
}

// wstring�� string���� ��ȯ�ϴ� �Լ� : �ܼ���ȯ
std::string WstrToStr(const std::wstring& source)
{
    return std::string().assign(source.begin(), source.end());
}

// wstring�� ASCII�� ��ȯ�ϴ� �Լ� : 0x0080 ������ ���ڸ� �߰�
std::string wstringToAscii(const std::wstring& wstr) {
    std::string asciiStr;
    for (wchar_t wc : wstr) {
        if (wc < 0x0080) { // ASCII ���� ���� ���ڸ� �߰�
            asciiStr += static_cast<char>(wc);
        }
        else {
            // ��ASCII ���ڸ� ��ü ���ڳ� ����
            // ��: asciiStr += '?'; �Ǵ� �׳� ����
        }
    }
    return asciiStr;
}

// string ���� wstring���� ��ȯ�ϴ� �Լ� : codecvt ���
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// string ���� wstring���� ��ȯ�ϴ� �Լ� : MultiByteToWideChar ���
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

// string ���� WCHAR* �� ��ȯ�ϴ� �Լ�
void StringToWChar(const std::string& s, WCHAR* outStr) {
    // ��ȯ�� ���ڿ��� ���̸� ����մϴ�. ��Ƽ����Ʈ�� ���̵� ���ڷ� ��ȯ�� �� �ʿ��� ���̸� ����ϴ�.
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);

    // ���� ��ȯ�� �����մϴ�.
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, outStr, len);
}

// EUC-KR ���ڿ��� UTF-16���� ��ȯ�մϴ�.
std::wstring EucKrToWString(const std::string& eucKrStr) {
    if (eucKrStr.empty()) return std::wstring();

    // EUC-KR�� �����ڵ�� ��ȯ�� �� ����ϴ� �ڵ� ������ ��ȣ�� 949�Դϴ�.
    const UINT codePage = 949;

    // �ʿ��� ���̵� ������ ���� ����մϴ�.
    int charsNeeded = MultiByteToWideChar(codePage, 0, eucKrStr.c_str(), -1, NULL, 0);
    if (charsNeeded == 0) {
        throw std::runtime_error("Failed converting EUC-KR string to UTF-16");
    }

    std::vector<wchar_t> buffer(charsNeeded);
    int charsConverted = MultiByteToWideChar(codePage, 0, eucKrStr.c_str(), -1, buffer.data(), charsNeeded);

    if (charsConverted == 0) {
        throw std::runtime_error("Failed converting EUC-KR string to UTF-16");
    }

    // ���ۿ��� �� ���� ���ڸ� �����ϰ� std::wstring ��ü�� �����մϴ�.
    return std::wstring(buffer.begin(), buffer.end() - 1);
}

// std::string (UTF-8)�� std::wstring (WCHAR �迭)���� ��ȯ�ϴ� �Լ�
std::wstring StringToWCHAR(const std::string& utf8String) {
    if (utf8String.empty()) return std::wstring();

    // UTF-8 ���ڿ��� WCHAR �迭�� ��ȯ�ϱ� ���� �ʿ��� ũ�� ���
    int wcharCount = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, NULL, 0);
    if (wcharCount == 0) {
        throw std::runtime_error("Failed to convert UTF-8 string to WCHAR.");
    }

    // WCHAR �迭�� ���� ���� �Ҵ�
    std::vector<WCHAR> wcharArray(wcharCount);

    // UTF-8 ���ڿ��� WCHAR �迭�� ��ȯ
    if (MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &wcharArray[0], wcharCount) == 0) {
        throw std::runtime_error("Failed to convert UTF-8 string to WCHAR.");
    }

    // std::wstring���� ��ȯ�Ͽ� ��ȯ
    return std::wstring(wcharArray.begin(), wcharArray.end() - 1); // null ���� ���� ����
}

// WCHAR�� std::string (UTF-8)���� ��ȯ�ϴ� �Լ�
std::string WCHARToString2(const WCHAR* wcharArray) {
    if (!wcharArray) return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, &strTo[0], sizeNeeded, NULL, NULL);

    return strTo;
}

// WCHAR�� std::string (UTF-8)���� ��ȯ�ϴ� �Լ� : NULL�� ����
std::string WCHARToString(const WCHAR* wcharArray) {
    if (!wcharArray) return std::string();

    // �ʿ��� ���ڿ� ũ�⸦ ����մϴ� (�� ���� ���� ����).
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string(); // ��ȯ�� �����ϰų� �� ���ڿ��� ���

    // sizeNeeded���� �� ���� ���ڵ� ���ԵǾ� �����Ƿ�, �̸� ������ ũ��� ���ڿ��� �����մϴ�.
    std::string strTo(sizeNeeded - 1, 0); // �� ���� ���ڸ� ������ ũ��
    WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, &strTo[0], sizeNeeded, NULL, NULL);

    return strTo;
}


int wstringToInt(const std::wstring& wstr) {
    int number = 0;
    try {
        // std::stoi�� ����Ͽ� std::wstring�� int�� ��ȯ
        number = std::stoi(wstr);
    }
    catch (const std::invalid_argument& e) {
        // ��ȯ�� �� ���� ���
        std::wcerr << L"Invalid argument: " << e.what() << std::endl;
        // ������ ���� ó��
    }
    catch (const std::out_of_range& e) {
        // ��ȯ�� ���� int ������ ����� ���
        std::wcerr << L"Out of range: " << e.what() << std::endl;
        // ������ ���� ó��
    }
    return number;
}

void trace(const std::string& msg) {
    OutputDebugStringA(msg.c_str());
}