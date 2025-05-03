#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <Richedit.h>
#include <shellapi.h>
#include <WinUser.h>
#include <tlhelp32.h>
#include <mutex>
#include <windef.h>
#include <memory>
#include "json.hpp"
#include "resource.h"

// 전방 선언
class NemoViewer;
class CDlgSummary;
class CDlgInfo;
class CDlgTrans;
class BgJob;

#ifndef LEFT_MENU_WIDTH
#define LEFT_MENU_WIDTH 43
#endif

class RealTrans {
public:
    RealTrans();
    ~RealTrans();

    // 윈도우 클래스 등록 및 초기화
    ATOM RegisterWindowClass(HINSTANCE hInstance);
    BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);

    // 메인 애플리케이션 실행
    int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

    // 윈도우 프로시저 (WindowProc Thunk)
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // 인스턴스 윈도우 프로시저
    LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // 대화 상자 콜백 (트램폴린)
    static INT_PTR CALLBACK StaticAboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // 인스턴스 대화 상자 콜백 
    INT_PTR AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // NemoViewer 설정 및 조작
    void SetNemoViewerConfig();
    void ScrollToBottom();
    void ScrollToBottomSel();
    void AddTextToViewer(const WCHAR* textToAdd);

    // 윈도우 크기 및 위치 조정
    void ResizeWindow(int width, int height);

    // 메뉴 표시/숨김 제어
    void ShowMenu(bool show);
    void SetStopMenu(bool stop) { m_bStopMenu = stop; }
    bool GetStopMenu() { return m_bStopMenu; }

    // 설정 불러오기
    void LoadSettings(const std::string& filePath);

    // 현재 실행 경로 설정
    void SetExecutePath();
    std::string GetExecutePath() { return m_strExecutePath;  }

    // Summary 관련
    void SetSummaryFont();

    // 싱글톤 접근자
    static RealTrans* GetInstance();

private:
    // 윈도우 핸들 및 인스턴스
    HWND m_hWnd;
    HINSTANCE m_hInst;

    // NemoViewer 객체
    std::unique_ptr<NemoViewer> m_pViewer;

    // 다이얼로그 객체들
    CDlgSummary* m_pDlgSum;
    CDlgInfo* m_pDlgInfo;
    CDlgTrans* m_pDlgTrans;

    // 백그라운드 작업 관리자
    BgJob* m_pBgJob;

    // 애플리케이션 상태 변수
    bool m_isTransWinActive;
    bool m_isMouseInside;
    bool m_bStopMenu;
    bool m_bMenuHide;
    bool m_bSetupActive;

    // 실행 경로
    std::string m_strExecutePath;

    // 타이머 ID
    static const UINT TIMER_ID = 1;

    // 아이콘 영역 정보
    RECT m_iconRect;

    // UI 언어 
    LPWSTR m_oldUILang;

    // 싱글톤 인스턴스
    static RealTrans* s_instance;
};