// ������ִ� richedit dialogue control class
#include <iostream>
#include <codecvt>
#include "StrConvert.h"
#include "json.hpp" // �Ǵ� ��θ� �����ؾ� �� ���: #include "External/json.hpp"
#include "CDlgSummary.h"

// nlohmann ���̺귯���� ���ӽ����̽� ���
using json = nlohmann::json;
extern CDlgSummary* DlgSum;

int CharToWChar(wchar_t* pwstrDest, const char* pstrSrc)
{
    size_t cn;
    setlocale(LC_ALL, "");

    // �Է� ���ڿ��� ���̸� ����մϴ�. ��ȯ�Ǵ� ���̿��� �� ���� ���ڰ� ���Ե��� �ʽ��ϴ�.
    int nLen = MultiByteToWideChar(CP_ACP, 0, pstrSrc, -1, NULL, 0);

    // ���� ��ȯ�� �����մϴ�. CP_ACP (ANSI Code Page)-locale�� ����� �� ������ ���� ���ڵ����, CP_UTF8 (UTF-8 Code Page)- ���� ȣȯ
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
    // UTF-8 ���ڿ��� ���̸� ����մϴ�. -1�� �� ���� ���ڸ� �����ϵ��� �մϴ�.
    int wideCharCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);

    // ����� ũ���� wstring ���۸� �غ��մϴ�.
    std::wstring wideStr(wideCharCount, 0);

    // UTF-8 ���ڿ��� UTF-16���� ��ȯ�մϴ�.
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharCount);

    return wideStr;
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

// std::string (UTF-8)�� std::wstring (WCHAR �迭)���� ��ȯ�ϴ� �Լ�
// string ���� wstring���� ��ȯ�ϴ� �Լ� : MultiByteToWideChar ���
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

// string�� wstring�� ��
BOOL CompareStringAndWString(const std::string& str, const std::wstring& wstr) {
    // wstring�� string���� ��ȯ
    std::string convertedStr = WStringToString(wstr);
    // ��ȯ�� string�� ���� string�� ��
    return str == convertedStr;
}

void trace(const std::string& msg) {
    OutputDebugStringA(msg.c_str());
}

// �ý����� ���� �ڵ� ���������� UTF-8 ���ڿ��� ��ȯ
std::string ConvertToUTF8(const std::string& input) {
    // ����, �Է� ���ڿ��� UTF-16���� ��ȯ
    int wcharCount = MultiByteToWideChar(GetConsoleCP(), 0, input.c_str(), -1, NULL, 0);
    std::vector<WCHAR> wstr(wcharCount);
    MultiByteToWideChar(GetConsoleCP(), 0, input.c_str(), -1, &wstr[0], wcharCount);

    // �� ����, UTF-16 ���ڿ��� UTF-8�� ��ȯ
    int utf8Count = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
    std::vector<char> utf8Str(utf8Count);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &utf8Str[0], utf8Count, NULL, NULL);

    return std::string(utf8Str.begin(), utf8Str.end() - 1); // null ���� ���� ����
}

std::string ConvertBinaryToString(const std::string& binaryData) {
    // ���̳ʸ� �����Ϳ��� b' '�� ���� �κ��� ã�Ƴ��ϴ�.
    size_t start = binaryData.find("b'");
    size_t end = binaryData.rfind("'");

    if (start == std::string::npos) {
        start = binaryData.find("b\"");
        end = binaryData.rfind("\"");
	}

    if (start != std::string::npos && end != std::string::npos && start < end) {
        // b' '�� ���� ������ �����մϴ�.
        std::string trimmed = binaryData.substr(start + 2, end - start - 2);

        // ����� ���ڿ����� �齽���� �̽������� �������� ���� ����Ʈ ������ ��ȯ�մϴ�.
        std::string result;
        for (size_t i = 0; i < trimmed.length(); ++i) {
            // ���� filter ( \n, \t, \r, \xHH )
            if (trimmed[i] == '\n' || trimmed[i]=='\r') {
                i++;
                continue;
            }
            if (trimmed[i] == '\\' && i + 3 < trimmed.length() && trimmed[i + 1] == 'x') {
                // \xHH ������ 16���� ���� ���ڷ� ��ȯ�մϴ�.
                std::string hexValue = trimmed.substr(i + 2, 2);
                char byteValue = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                result.push_back(byteValue);
                i += 3; // 16���� ���� �̽������� �������� �ǳʶݴϴ�.
            }
            else {
                result.push_back(trimmed[i]);
            }
        }
        return result;
    }

    // b' ' ������ �ƴ϶�� ���� �����͸� �״�� ��ȯ�մϴ�.
    return binaryData;
}

std::wstring ConvertBinaryToWString(const std::string& binaryData) {
    // ���̳ʸ� �����Ϳ��� b' '�� ���� �κ��� �����մϴ�.
    size_t start = binaryData.find("b'");
    size_t end = binaryData.rfind("'");
    std::string trimmed;

    if (start != std::string::npos && end != std::string::npos && start < end) {
        trimmed = binaryData.substr(start + 2, end - start - 2);
    }
    else {
        // b' ' ������ �ƴϸ� ���� �����͸� ����մϴ�.
        trimmed = binaryData;
    }

    // UTF-8 ���ڿ��� UTF-16���� ��ȯ�մϴ�.
    int wlen = MultiByteToWideChar(CP_UTF8, 0, trimmed.c_str(), -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, trimmed.c_str(), -1, &wstr[0], wlen);

    return wstr;
}