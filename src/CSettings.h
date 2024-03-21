#pragma once
#include <windows.h>
#include <string>

// 언어와 관련 데이터를 저장하는 구조체 정의
struct LanguageData {
    LPCWSTR language;
    LPCWSTR data;
};

void ReadSettings(const std::string& filePath);
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CSettings
{
};

