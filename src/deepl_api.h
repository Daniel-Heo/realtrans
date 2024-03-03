#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>

void RequestDeepL(const std::string& text, std::string& strOut, const std::string& lang, const std::string& api_key);
