#pragma once
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <windows.h>
#include <iostream>

int GetSoundDev(WCHAR* resStr);
int GetActiveSound(WCHAR* resStr);