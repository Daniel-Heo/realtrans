#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

class CDlgInfo {
public:
    HWND hDlgInfo; // Dialog�� �ڵ�
    HINSTANCE hInstance; // ���ø����̼� �ν��Ͻ� �ڵ�
    HWND hWndParent; // �θ� �������� �ڵ�
    BOOL bVisible; // ���â�� ���̴��� ����
    BOOL bExistDlg; // ���â�� �����ϴ��� ����

public:
    CDlgInfo(HINSTANCE hInst);

    // RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
    void Create(HWND hWndParent);

    void Alert(const WCHAR* msg); // ���â���� ����� ��� ���

    // ���â ����� ( �⺻������ Ȱ��ȭ�Ǹ� �� �������ʹ� ������ ���̱�� ���� )
    void Hide();
    void Show();

};