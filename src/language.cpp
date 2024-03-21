//#include "pch.h"
#include "language.h"
#include <iostream>

// UI ��� �ڵ� �������� : ko_KR, en_US
void GetUserLocale(LPWSTR* ppUILang)
{
    ULONG numLangs = 0;
    ULONG lenBuffer = 0;
    LPWSTR pOrgUILang = NULL;

    if (!GetThreadPreferredUILanguages(0, &numLangs, NULL, &lenBuffer))  return;

    pOrgUILang = (LPWSTR)malloc(sizeof(wchar_t) * lenBuffer);
    if (pOrgUILang == NULL)  return;
    
    if (!GetThreadPreferredUILanguages(0, &numLangs, pOrgUILang, &lenBuffer))
    {
        // release the buffer
        free(pOrgUILang);
        pOrgUILang = NULL;
        return;
    }

    *ppUILang = pOrgUILang;
}

// UI �ٱ��� ����
void SetUserLocale(const WCHAR* newUILang)
{
    ULONG numLangs = 0;
    ULONG lenBuffer = 0;

    if (!GetThreadPreferredUILanguages(0, &numLangs, NULL, &lenBuffer))
        return;

    // Set the prefferred UI locale and ignore the result
    SetThreadPreferredUILanguages(0, newUILang, &numLangs);
    setlocale(LC_ALL, "UTF-8"); // setlocale(LC_ALL, "ko_KR.UTF-8");
}

// UI ��Ķ �޸� ����
void ResetUserLocale(LPWSTR pOrgUILang)
{
    if (NULL != pOrgUILang)
    {
        ULONG numLangs = 0;
        SetThreadPreferredUILanguages(0, pOrgUILang, &numLangs);
        free(pOrgUILang);
    }
}