/* ---------------------------------------
* Util.h
 * 특정 사용목적이 없는 함수들과 디버그용 함수들로 이루어짐
 -----------------------------------------*/
#pragma once
#include <windows.h>
#include <string>

COLORREF HexToCOLORREF(const std::string& hexCode);

// DEBUG용
void DebugInt(HWND hEdit, int value);
void TRACE(const TCHAR* format, ...);
void TRACE(const CHAR* format, ...);