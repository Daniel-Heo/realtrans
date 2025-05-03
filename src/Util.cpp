#include "Util.h"
#include <tchar.h>

// HEX 색상 코드를 COLORREF로 변환
COLORREF HexToCOLORREF(const std::string& hexCode) {
    // 색상 코드에서 '#' 문자를 제거합니다.
    std::string hex = hexCode.substr(1);

    // 16진수 값을 10진수로 변환합니다.
    int r = std::stoi(hex.substr(0, 2), nullptr, 16);
    int g = std::stoi(hex.substr(2, 2), nullptr, 16);
    int b = std::stoi(hex.substr(4, 2), nullptr, 16);

    // COLORREF 값으로 변환합니다. RGB 매크로를 사용합니다.
    return RGB(r, g, b);
}

// int 값을 요약창에 보이게
void DebugInt(HWND hEdit, int value)
{
    WCHAR buf[200];
    wsprintf(buf, L"int: [%d]", value);
    SendMessage(hEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
}

void TRACE(const TCHAR* format, ...)
{
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    _vsntprintf_s(buffer, sizeof(buffer) / sizeof(TCHAR), _TRUNCATE, format, args);
    va_end(args);
    OutputDebugString(buffer);
}

void TRACE(const CHAR* format, ...)
{
    CHAR utf8Buffer[1024] = { 0 };
    va_list args;
    va_start(args, format);
    _vsnprintf_s(utf8Buffer, sizeof(utf8Buffer) - 1, _TRUNCATE, format, args);
    va_end(args);

    // CP_UTF8을 사용하여 현재 코드 페이지로 변환
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Buffer, -1, NULL, 0);
    WCHAR* wBuf = new WCHAR[len];
    MultiByteToWideChar(CP_UTF8, 0, utf8Buffer, -1, wBuf, len);

    OutputDebugStringW(wBuf);
    delete[] wBuf;
}