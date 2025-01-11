#pragma once
#include <windows.h>
#include <string>
#include "SoundDev.h"

// 언어와 관련 데이터를 저장하는 구조체 정의
struct LanguageData {
    LPCWSTR language;
    LPCWSTR data;
};

void ReadSettings(const std::string& filePath);
void MakeChildCmd(const std::string& src_lang, const std::string& tgt_lang); // Python에게 명령 전달
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SaveSimpleSettings(); // 간단한 설정 저장

class CSettings
{
};

