#include "SoundDev.h"

int GetSoundDev(WCHAR *resStr)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "COM ���̺귯�� �ʱ�ȭ ����" << std::endl;
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cout << "IMMDeviceEnumerator �ν��Ͻ� ���� ����" << std::endl;
        CoUninitialize();
        return -1;
    }

    IMMDeviceCollection* pCollection = NULL;
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        std::cout << "����� ��������Ʈ ���� ����" << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    UINT count;
    pCollection->GetCount(&count);
    if (count == 0) {
        std::cout << "Ȱ�� ����� ��ġ ����" << std::endl;
        return -1;
    }

    wcscpy_s(resStr, 200,  L"");
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
                    //printf("��ġ %d: %S\n", i, varName.pwszVal);
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

int GetActiveSound(WCHAR* resStr)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "COM ���̺귯�� �ʱ�ȭ ����" << std::endl;
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cout << "IMMDeviceEnumerator �ν��Ͻ� ���� ����" << std::endl;
        CoUninitialize();
        return -1;
    }

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cout << "�⺻ ����� ��������Ʈ ��ȸ ����" << std::endl;
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
        //printf("���� ������� ��ġ [%s].\n", varName.pwszVal);
        //std::wcout << L"���� ��� ���� ����� ��� ��ġ: " << varName.pwszVal << std::endl;
        //std::cout << "���� ��� ���� ����� ��� ��ġ: " << varName.pwszVal << std::endl;
        //printf("��ġ  %S\n",  varName.pwszVal);
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