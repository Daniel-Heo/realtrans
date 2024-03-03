#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

class CDlgSummary {
public:
    HWND hDlgSummary; // Dialog�� �ڵ�
    HWND hSumRichEdit; // RichEdit ��Ʈ���� �ڵ�
    HINSTANCE hInstance; // ���ø����̼� �ν��Ͻ� �ڵ�
    HWND hWndParent; // �θ� �������� �ڵ�
    int nParentRichPos; // �θ� �������� RichEdit ��Ʈ���� ���� ��ġ ( ������� �ʴ´� -> strEng���� ������ )
    BOOL bVisible; // ���â�� ���̴��� ����
    BOOL bExistDlg; // ���â�� �����ϴ��� ����
    
public:
    CDlgSummary(HINSTANCE hInst);

    // RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
    void Create(HWND hWndParent);

    // RichEdit ��Ʈ�ѿ� �ؽ�Ʈ ����
    void SetText(const wchar_t* text);

    // RichEdit ��Ʈ�ѿ��� �ؽ�Ʈ ��������
    void GetText(wchar_t* buffer, int bufferSize);

    // tokken size üũ
    int getTokenSize(const std::string& wstr, int curPos);

    // Handle�� RichEdit ��Ʈ�ѿ��� �ؽ�Ʈ ��������
    BOOL GetTextWithHandle(std::string& buf);

    // RichEdit ��Ʈ�ѿ� �ؽ�Ʈ �߰�
    void AddText(const std::wstring& text);

    // RichEdit ��Ʈ�ѿ� �ؽ�Ʈ ������ ��������
    int GetTextSize(HWND hRich);

    // ��� ��ư Ŭ���� ��׶���� ���
    BOOL BgSummary();

    // RichEdit�� �ؽ�Ʈ �߰�
    BOOL AddRichText();

    // Traceâ���� ����� ��� ���
    BOOL Trace(const std::wstring& trace);

    void Alert(const WCHAR* msg); // ���â���� ����� ��� ���

    // ���â ����� ( �⺻������ Ȱ��ȭ�Ǹ� �� �������ʹ� ������ ���̱�� ���� )
    void Hide();
    void Show();

    // ���â�� �������� ��ġ�� ���� ���������� ���̰� �̵�
    void GoBottom();

    // #c8c8c8 ������ COLORREF�� ��ȯ
    COLORREF HexToCOLORREF(const std::string& hexCode);

    // ��Ʈ ����
    void SetFont();
    
};