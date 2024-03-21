#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

class CDlgInfo {
public:
    HWND hDlgInfo; // Dialog의 핸들
    HINSTANCE hInstance; // 애플리케이션 인스턴스 핸들
    HWND hWndParent; // 부모 윈도우의 핸들
    BOOL bVisible; // 요약창이 보이는지 여부
    BOOL bExistDlg; // 요약창이 존재하는지 여부

public:
    CDlgInfo(HINSTANCE hInst);

    // RichEdit 컨트롤을 생성하고 초기화하는 함수
    void Create(HWND hWndParent);

    void Alert(const WCHAR* msg); // 경고창으로 사용할 경우 사용

    // 요약창 숨기기 ( 기본적으로 활성화되면 그 다음부터는 숨기기와 보이기로 제어 )
    void Hide();
    void Show();

};