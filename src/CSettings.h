#pragma once
#include <windows.h>
#include <string>

void ReadSettings(const std::string& filePath);
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CSettings
{
};

