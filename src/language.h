#pragma once
#include <windows.h>

void GetUserLocale(LPWSTR* ppUILang); // UI ��Ķ ��������
void SetUserLocale(const WCHAR* newUILang); // UI ��Ķ ����
void ResetUserLocale(LPWSTR pOrgUILang); // UI ��Ķ �޸� ����
