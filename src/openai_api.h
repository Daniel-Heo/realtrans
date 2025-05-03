#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>

void RequestOpenAIChat(const std::string& text, std::wstring& strOut, const std::string& lang, const std::string& hint, const WCHAR* api_key);
