#include "SoundDev.h"
#include <Audioclient.h>

// 샘플링 레이트를 포함하여 현재 활성 사운드 장치의 기본 형식을 얻습니다.
HRESULT GetDeviceFormat(IMMDevice* pDevice, WAVEFORMATEX** ppWaveFormatEx) {
    IAudioClient* pAudioClient = NULL;
    HRESULT hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) {
        std::cout << "IAudioClient 활성화 실패" << std::endl;
        return hr;
    }

    hr = pAudioClient->GetMixFormat(ppWaveFormatEx);
    if (FAILED(hr)) {
        std::cout << "기본 오디오 형식 조회 실패" << std::endl;
    }
    pAudioClient->Release();

    return hr;
}

// 현재 활성 사운드 장치의 이름을 가져옵니다.
int GetActiveSound(WCHAR* resStr, WAVEFORMATEX* pWaveFormat)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "COM 라이브러리 초기화 실패" << std::endl;
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cout << "IMMDeviceEnumerator 인스턴스 생성 실패" << std::endl;
        CoUninitialize();
        return -1;
    }

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cout << "기본 오디오 엔드포인트 조회 실패" << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }
    else {
        WAVEFORMATEX* pWaveFormatEx = NULL;

        HRESULT hr = GetDeviceFormat(pDevice, &pWaveFormatEx);
        if (!SUCCEEDED(hr)) {
			pDevice->Release();
			pEnumerator->Release();
			CoUninitialize();
			return -1;
        }

        memcpy(pWaveFormat, pWaveFormatEx, sizeof(WAVEFORMATEX));

        if (pWaveFormatEx) {
            CoTaskMemFree(pWaveFormatEx);
        }
    }

    IPropertyStore* pProps = NULL;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (FAILED(hr)) {
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    PROPVARIANT varName;
    PropVariantInit(&varName);
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    if (SUCCEEDED(hr)) {
        //printf("현재 사용중인 장치 [%s].\n", varName.pwszVal);
        //std::wcout << L"현재 사용 중인 오디오 출력 장치: " << varName.pwszVal << std::endl;
        //std::cout << "현재 사용 중인 오디오 출력 장치: " << varName.pwszVal << std::endl;
        //printf("장치  %S\n",  varName.pwszVal);
        //wprintf(TEXT("%s\n"), varName.pwszVal);
        wcscpy_s(resStr, 200, varName.pwszVal);
        PropVariantClear(&varName);
    }
    else return -1;


    pProps->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    return TRUE;
}

// 기본 사운드 출력 장치의 이름을 가져옴
int GetDefaultSound(WCHAR* resStr)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "COM 라이브러리 초기화 실패" << std::endl;
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cout << "IMMDeviceEnumerator 인스턴스 생성 실패" << std::endl;
        CoUninitialize();
        return -1;
    }

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cout << "기본 오디오 엔드포인트 조회 실패" << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    IPropertyStore* pProps = NULL;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (FAILED(hr)) {
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    PROPVARIANT varName;
    PropVariantInit(&varName);
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    if (SUCCEEDED(hr)) {
        //printf("현재 사용중인 장치 [%s].\n", varName.pwszVal);
        //std::wcout << L"현재 사용 중인 오디오 출력 장치: " << varName.pwszVal << std::endl;
        //std::cout << "현재 사용 중인 오디오 출력 장치: " << varName.pwszVal << std::endl;
        //printf("장치  %S\n",  varName.pwszVal);
        //wprintf(TEXT("%s\n"), varName.pwszVal);
        wcscpy_s(resStr, 200, varName.pwszVal);
        PropVariantClear(&varName);
    }
    else return -1;


    pProps->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    return TRUE;
}

// 장치 정보를 가져와 저장하는 함수
int GetAudioDevices(WCHAR *devices[20]) {
    // COM 라이브러리 초기화
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::wcerr << L"COM 라이브러리 초기화 실패: " << hr << L"\n";
        return 0; // 초기화 실패 시 0 반환
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDeviceCollection* pCollection = nullptr;
    IMMDevice* pDevice = nullptr;
    IPropertyStore* pProps = nullptr;

    int deviceCount = 0; // 발견된 장치 개수

    do {
        // IMMDeviceEnumerator 생성
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) {
            std::wcerr << L"IMMDeviceEnumerator 생성 실패: " << hr << L"\n";
            break;
        }

        // 출력 장치(렌더 디바이스) 나열
        hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) {
            std::wcerr << L"출력 장치 나열 실패: " << hr << L"\n";
            break;
        }

        UINT count = 0;
        hr = pCollection->GetCount(&count);
        if (FAILED(hr)) {
            std::wcerr << L"출력 장치 개수 확인 실패: " << hr << L"\n";
            break;
        }

        for (UINT i = 0; i < count && deviceCount < 20; ++i) {
            hr = pCollection->Item(i, &pDevice);
            if (FAILED(hr)) {
                std::wcerr << L"장치 가져오기 실패 (출력 장치): " << hr << L"\n";
                continue;
            }

            // 장치 속성 가져오기
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (FAILED(hr)) {
                std::wcerr << L"장치 속성 가져오기 실패: " << hr << L"\n";
                pDevice->Release();
                continue;
            }

            PROPVARIANT varName;
            PropVariantInit(&varName);

            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
				devices[deviceCount] = new WCHAR[100]; // 장치 이름 저장
                wcsncpy_s(devices[deviceCount], 100, varName.pwszVal, _TRUNCATE); // 장치 이름 저장
                deviceCount++;
            } else {
                std::wcerr << L"장치 이름 가져오기 실패: " << hr << L"\n";
            }

            PropVariantClear(&varName);
            pProps->Release();
            pDevice->Release();
        }

        pCollection->Release();

        // 입력 장치(캡처 디바이스) 나열
        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) {
            std::wcerr << L"입력 장치 나열 실패: " << hr << L"\n";
            break;
        }

        hr = pCollection->GetCount(&count);
        if (FAILED(hr)) {
            std::wcerr << L"입력 장치 개수 확인 실패: " << hr << L"\n";
            break;
        }

        for (UINT i = 0; i < count && deviceCount < 20; ++i) {
            hr = pCollection->Item(i, &pDevice);
            if (FAILED(hr)) {
                std::wcerr << L"장치 가져오기 실패 (입력 장치): " << hr << L"\n";
                continue;
            }

            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (FAILED(hr)) {
                std::wcerr << L"장치 속성 가져오기 실패: " << hr << L"\n";
                pDevice->Release();
                continue;
            }

            PROPVARIANT varName;
            PropVariantInit(&varName);

            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
                devices[deviceCount] = new WCHAR[100]; // 장치 이름 저장
                wcsncpy_s(devices[deviceCount], 100, varName.pwszVal, _TRUNCATE); // 장치 이름 저장
                deviceCount++;
            } else {
                std::wcerr << L"장치 이름 가져오기 실패: " << hr << L"\n";
            }

            PropVariantClear(&varName);
            pProps->Release();
            pDevice->Release();
        }
    } while (0);

    // 자원 해제
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();

    return deviceCount;
}

// 사용하지 않음
// 
// 활성화된 마이크 장치의 이름을 가져오는 함수 : 첫번째 마이크만 가져오더라.
// 성공 시 0 반환, 실패 시 -1 반환
//BOOL GetActiveMic(WCHAR* resStr) {
//    // 시스템에 연결된 입력 장치의 개수를 가져옵니다.
//    UINT numDevices = waveInGetNumDevs();
//
//    // 입력 장치가 하나도 없으면 실패(-1)를 반환합니다.
//    if (numDevices == 0) {
//        std::wcerr << L"마이크 장치가 발견되지 않았습니다." << std::endl;
//        return FALSE;
//    }
//
//    // 각 입력 장치의 정보를 순회하며 활성화된 장치를 찾습니다.
//    for (UINT i = 0; i < numDevices; ++i) {
//        WAVEINCAPS waveInCaps;
//
//        // 입력 장치의 정보를 가져옵니다.
//        MMRESULT result = waveInGetDevCaps(i, &waveInCaps, sizeof(WAVEINCAPS));
//
//        // 정보를 성공적으로 가져온 경우
//        if (result == MMSYSERR_NOERROR) {
//            // 활성화된 장치로 판단하고 장치 이름을 resStr에 저장합니다.
//            // 이 예제에서는 첫 번째 장치를 활성화된 것으로 가정합니다.
//            wcscpy_s(resStr, MAXPNAMELEN, waveInCaps.szPname);
//            return TRUE; // 성공적으로 장치 이름을 가져온 경우 0 반환
//        }
//        else {
//            std::wcerr << L"장치 정보를 가져오는 중 오류가 발생했습니다. 오류 코드: " << result << std::endl;
//        }
//    }
//
//    return FALSE; // 장치를 찾지 못한 경우 -1 반환
//}

int GetSoundDev(WCHAR* resStr)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "COM 라이브러리 초기화 실패" << std::endl;
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cout << "IMMDeviceEnumerator 인스턴스 생성 실패" << std::endl;
        CoUninitialize();
        return -1;
    }

    IMMDeviceCollection* pCollection = NULL;
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        std::cout << "오디오 엔드포인트 열거 실패" << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    UINT count;
    pCollection->GetCount(&count);
    if (count == 0) {
        std::cout << "활성 오디오 장치 없음" << std::endl;
        return -1;
    }

    wcscpy_s(resStr, 200, L"");
    for (UINT i = 0; i < count; i++) {
        IMMDevice* pDevice = NULL;
        hr = pCollection->Item(i, &pDevice);
        if (SUCCEEDED(hr)) {
            IPropertyStore* pProps = NULL;
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (SUCCEEDED(hr)) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                if (SUCCEEDED(hr)) {
                    //printf("장치 %d: %S\n", i, varName.pwszVal);
                    //sprintf_s(resStr, 100, "%d: %S", i, varName.pwszVal);
                    wcscat_s(resStr, 200, varName.pwszVal);
                    wcscat_s(resStr, 200, L",");
                    PropVariantClear(&varName);
                }
                pProps->Release();
            }
            pDevice->Release();
        }
    }

    pCollection->Release();
    pEnumerator->Release();
    CoUninitialize();

    return TRUE;
}