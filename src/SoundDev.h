#pragma once
// 소리 출력 장치 확인을 위해
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <windows.h>
#include <iostream>

// 사운드 장치의 정보를 가져옴
HRESULT GetDeviceFormat(IMMDevice* pDevice, WAVEFORMATEX** ppWaveFormatEx);
// 활성화된 소리 장치의 이름을 가져옴
int GetActiveSound(WCHAR* resStr, WAVEFORMATEX* pWaveFormat);
// 기본 사운드 출력 장치의 이름을 가져옴
int GetDefaultSound(WCHAR* resStr);
// 사운드 장치들과 마이크 장치들을 가져옴
int GetAudioDevices(WCHAR *devices[20]);
// 사용하지 않음
int GetSoundDev(WCHAR* resStr);