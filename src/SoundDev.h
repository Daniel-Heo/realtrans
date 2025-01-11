#pragma once
// �Ҹ� ��� ��ġ Ȯ���� ����
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <windows.h>
#include <iostream>

// ���� ��ġ�� ������ ������
HRESULT GetDeviceFormat(IMMDevice* pDevice, WAVEFORMATEX** ppWaveFormatEx);
// Ȱ��ȭ�� �Ҹ� ��ġ�� �̸��� ������
int GetActiveSound(WCHAR* resStr, WAVEFORMATEX* pWaveFormat);
// �⺻ ���� ��� ��ġ�� �̸��� ������
int GetDefaultSound(WCHAR* resStr);
// ���� ��ġ��� ����ũ ��ġ���� ������
int GetAudioDevices(WCHAR *devices[20]);
// ������� ����
int GetSoundDev(WCHAR* resStr);