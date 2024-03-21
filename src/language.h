#pragma once
#include <windows.h>

void GetUserLocale(LPWSTR* ppUILang); // UI 로캘 가져오기
void SetUserLocale(const WCHAR* newUILang); // UI 로캘 설정
void ResetUserLocale(LPWSTR pOrgUILang); // UI 로캘 메모리 해제
