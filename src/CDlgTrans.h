#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

class CDlgTrans {
public:
    HINSTANCE hInstance; // ���ø����̼� �ν��Ͻ� �ڵ�
    HWND hWnd; // �θ� �������� �ڵ�
    BOOL bVisible; // ���â�� ���̴��� ����
    BOOL bExistDlg; // ���â�� �����ϴ��� ����
    BOOL isTrans; // ���������� Ȯ��

    // ������ ũ��
    int window_width;
    int window_height;

    // RichEdit ��Ʈ��
    HWND hRich1; // RichEdit ��Ʈ���� �ڵ�1
    HWND hRich2; // RichEdit ��Ʈ���� �ڵ�2

public:
    CDlgTrans(HINSTANCE hInst);

    BOOL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
    void InitDialogControls();
    void HideDialogControls();
    void ShowDialogControls();
    void Create(HWND hWndParent);
    void Destroy(); 

    void Translate(std::string& strOut, std::string& srcLang, std::string& tgtLang);
    void CheckTransFile();
    void ShowTransFile();


    void Alert(const WCHAR* msg); // ���â���� ����� ��� ���

    // ���â ����� ( �⺻������ Ȱ��ȭ�Ǹ� �� �������ʹ� ������ ���̱�� ���� )
    void Hide();
    void Show();

    void GetText(std::string& strOut, std::string& srcLang, std::string& tgtLang);

};