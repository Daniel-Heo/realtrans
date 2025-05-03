#pragma once
#include <windows.h>
#include <Richedit.h>
#include <string>
#include "resource.h"

#define BLOCK_4K 4096

class CDlgSummary {
public:
    HWND hDlgSummary; // Dialog의 핸들
    HWND hSumRichEdit; // RichEdit 컨트롤의 핸들
    HINSTANCE hInstance; // 애플리케이션 인스턴스 핸들
    HWND hWndParent; // 부모 윈도우의 핸들
    size_t nParentRichPos; // 부모 윈도우의 RichEdit 컨트롤의 현재 위치 ( 사용하지 않는다 -> strEng에서 가져옴 )
    BOOL bVisible; // 요약창이 보이는지 여부
    BOOL bExistDlg; // 요약창이 존재하는지 여부
    //bool addSummaryFlag;
public:
    CDlgSummary(HINSTANCE hInst);
    ~CDlgSummary();

    // 정적 다이얼로그 프로시저 (WindowProc Thunk)
    static INT_PTR CALLBACK StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 인스턴스 다이얼로그 프로시저
    INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // RichEdit 컨트롤을 생성하고 초기화하는 함수
    void Create(HWND hWndParent);

    // RichEdit 컨트롤에 텍스트 설정
    void SetText(const wchar_t* text);

    // RichEdit 컨트롤에서 텍스트 가져오기
    void GetText(wchar_t* buffer, int bufferSize);

    // 테이블 추가
    void InsertTable(const std::wstring& text);

    // tokken size 체크
    int getTokenSize(const std::string& wstr, int curPos);

    // RichEdit 컨트롤에 텍스트 추가
    void AddText(const std::wstring& text);

    // RichEdit 컨트롤에 텍스트 사이즈 가져오기
    int GetTextSize(HWND hRich);

    // 요약 버튼 클릭시 백그라운드로 요약
    BOOL BgSummary(std::string txt);

    // RichEdit에 텍스트 추가
    BOOL AddRichText(std::wstring txt);

    // Trace창으로 사용할 경우 사용
    BOOL Trace(const std::wstring& trace);

    void Alert(const WCHAR* msg); // 경고창으로 사용할 경우 사용

    // 요약창 숨기기 ( 기본적으로 활성화되면 그 다음부터는 숨기기와 보이기로 제어 )
    void Hide();
    void Show();

    // 요약창의 보여지는 위치를 제일 마지막줄이 보이게 이동
    void GoBottom();

    // 폰트 설정
    void SetFont();

private:

    // 다이얼로그 초기화
    void InitDialog(HWND hwndDlg);
};