#pragma once
// 소리 출력 장치 확인을 위해
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <windows.h>
#include <iostream>

int GetSoundDev(WCHAR* resStr);
int GetActiveSound(WCHAR* resStr);