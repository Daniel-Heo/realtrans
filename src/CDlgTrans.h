#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

class CDlgTrans {
public:
    HINSTANCE hInstance; // 애플리케이션 인스턴스 핸들
    HWND hWnd; // 부모 윈도우의 핸들
    BOOL bVisible; // 보이는지 여부
    BOOL bExistDlg; // 존재하는지 여부
    BOOL isTrans; // 번역중인지 확인
    // 번역 진행 %
    int transPercent;

    // 윈도우 크기
    int window_width;
    int window_height;

    // RichEdit 컨트롤
    HWND hRich1; // RichEdit 컨트롤의 핸들1
    HWND hRich2; // RichEdit 컨트롤의 핸들2

public:
    CDlgTrans(HINSTANCE hInst);

    BOOL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // RichEdit 컨트롤을 생성하고 초기화하는 함수
    void InitDialogControls();
    void HideDialogControls();
    void ShowDialogControls();
    void Create(HWND hWndParent);
    void Destroy(); 

    void Translate(std::string& strOut, std::string& srcLang, std::string& tgtLang);
    void CheckTransFile();
    void ShowTransFile();

    // 파일로 텍스트 입력 : txt, pdf
    BOOL InputFile();
    BOOL OutputFile();
    BOOL BrowseFile(WCHAR* szPath, WCHAR* szFile);
    BOOL BrowseSaveFile(WCHAR* szPath, WCHAR* szFile);

    void ProgressBar(int percent);


    void Alert(const WCHAR* msg); // 경고창으로 사용할 경우 사용

    // 요약창 숨기기 ( 기본적으로 활성화되면 그 다음부터는 숨기기와 보이기로 제어 )
    void Hide();
    void Show();

    void GetText(std::string& strOut, std::string& srcLang, std::string& tgtLang);

};