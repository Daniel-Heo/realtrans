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

// 사용하지 않음
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