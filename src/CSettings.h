#pragma once
#include <windows.h>
#include <string>
#include "SoundDev.h"

// ���� ���� �����͸� �����ϴ� ����ü ����
struct LanguageData {
    LPCWSTR language;
    LPCWSTR data;
};

void ReadSettings(const std::string& filePath);
void MakeChildCmd(const std::string& src_lang, const std::string& tgt_lang); // Python���� ��� ����
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SaveSimpleSettings(); // ������ ���� ����

class CSettings
{
};

