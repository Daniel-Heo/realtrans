//﻿*******************************************************************************
//    파     일     명 : NemoViewer.h
//    프로그램명칭 : 네모 뷰어
//    프로그램용도 : Windows API 기반의 텍스트 뷰어 컨트롤 (읽기 전용, 워드랩 고정)
//    참  고  사  항  : NemoEdit에서 변환됨, 단순화된 읽기 전용 모드로 동작
//
//    작    성    자 : Original by Daniel Heo
//    라 이  센 스  : MIT License
//    ----------------------------------------------------------------------------
//    수정일자    수정자      수정내용
//    =========== =========== ====================================================
//    2025.4.22   [Your Name] NemoEdit에서 Win32 버전으로 변환
//*******************************************************************************
#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <d2d1_1.h>
#include "D2Render.h"

#define LEFT_MENU_WIDTH 43

// 여백 구조체
struct Margin {
    int left;
    int right;
    int top;
    int bottom;
};

// 텍스트 위치 구조체
struct TextPos {
    int lineIndex;     // 라인 인덱스 (0부터 시작)
    int column;        // 열 인덱스 (0부터 시작)

    TextPos(int li = 0, int col = 0) : lineIndex(li), column(col) {}

    bool operator==(const TextPos& other) const {
        return lineIndex == other.lineIndex && column == other.column;
    }

    bool operator!=(const TextPos& other) const {
        return !(*this == other);
    }
};

// 선택 정보 구조체
struct SelectInfo {
    bool isSelected = false;     // 선택된 상태인지 여부
    bool isSelecting = false;    // 선택 중인지 여부
    TextPos start;               // 선택 시작 위치
    TextPos end;                 // 선택 끝 위치
    TextPos anchor;              // 선택 기준점 위치
};

// 색상 정보 구조체
struct ColorInfo {
    COLORREF text;
    COLORREF textBg;
    COLORREF oldText;
    COLORREF select;
};

// NemoViewer 클래스 정의
class NemoViewer {
public:
    NemoViewer();
    virtual ~NemoViewer();

    // 초기화 및 종료
    bool Initialize(HWND hwnd);
    void Shutdown();
    void Show();
    void Hide();

    // 설정 메서드
    void SetFont(const std::wstring& fontName, int fontSize, bool bold = false, bool italic = false);
    void GetFont(std::wstring& fontName, int& fontSize, bool& bold, bool& italic);
    void SetFontSize(int size);
    int GetFontSize();
    void SetLineSpacing(int spacing);
    void SetMargin(int left, int right, int top, int bottom);
    void SetTextColor(COLORREF textColor, COLORREF bgColor);
    void SetTextColor(COLORREF textColor);
    void SetOldTextColor(COLORREF textColor);
    void SetBgColor(COLORREF textBgColor);
    COLORREF GetTextColor();
    COLORREF GetTextBgColor();
    void SetLineNumColor(COLORREF lineNumColor, COLORREF bgColor); // TODO
    void SetMenuAreaWidth(int width);               // 라인 번호 영역 너비 설정
    // 배경 투명도 설정/조회 메서드 추가
    void SetBgOpacity(float opacity);
    float GetBgOpacity() const;

    // 텍스트 관련 메서드
    void SetText(const std::wstring& text);
    void AddText(std::wstring text);
    std::wstring GetText();
    std::wstring GetSelectedText();
    void Clear();
    size_t GetLineCount() const;
    void GotoLine(size_t lineNo);
    void SelectAll();
    void ClearSelection();

    // 윈도우 메시지 처리
    LRESULT ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // 그리기 관련
    void Paint(HDC hdc);
    void Resize(int width, int height);

    // 메뉴 상태 설정 메서드 
    void SetMenuState(bool bStopMenu);
    bool LoadMenuImages();

    D2Render* GetD2Render() { return &m_d2Render; }
    void SetDwmExtend();
    void ResetDwmExtend();

private:
    // 내부 데이터
    std::vector<std::wstring> m_lines;     // 텍스트 라인 저장 벡터
    D2Render m_d2Render;                   // Direct2D 렌더링 객체
    HWND m_hwnd;                          // 부모 윈도우 핸들
	bool m_isShow;           // 표시 여부 플래그

    // 캐럿/선택 관련
    TextPos m_caretPos;                   // 캐럿 위치
    SelectInfo m_selectInfo;              // 선택 영역 정보

    // 설정 관련
    int m_wordWrapWidth;                  // 자동 줄바꿈 너비
    int m_lineSpacing;                    // 추가 줄 간격
    int m_lineHeight;                     // 한 라인의 높이
    int m_charWidth;                      // 문자 폭
    Margin m_margin;                      // 여백
    ColorInfo m_colorInfo;                // 색상 정보

    // 스크롤 관련
    int m_scrollYLine;                    // 수직 스크롤 시작 라인
    int m_scrollYWrapLine;                // 워드랩 모드에서 수직 스크롤 시작 위치

    // 라인 번호 관련
    mutable int m_menuAreaWidth;        // 라인 번호 영역 너비
    
    // 마우스 이벤트 관련
    POINT m_lastClickPos;                 // 마지막 클릭 위치
    DWORD m_lastClickTime;                // 마지막 클릭 시간
    int m_clickCount;                     // 클릭 카운트

    // 메뉴 이미지 관련 변수
    ID2D1Bitmap* m_pMenuNormalBitmap;
    ID2D1Bitmap* m_pMenuOverBitmap;
    bool m_bStopMenu;

    // 내부 유틸리티 함수
    std::vector<int> FindWordWrapPosition(int lineIndex);  // 워드랩 위치 계산
    POINT GetCaretPixelPos(const TextPos& pos);          // 텍스트 위치를 픽셀 좌표로 변환
    TextPos GetTextPosFromPoint(POINT point);            // 픽셀 좌표를 텍스트 위치로 변환
    void EnsureCaretVisible();                            // 캐럿이 화면에 표시되도록 스크롤 조정
    void RecalcScrollSizes();                            // 스크롤 크기 재계산
    void SplitTextByNewlines(const std::wstring& text, std::vector<std::wstring>& parts);  // 텍스트를 줄바꿈으로 분리
    std::wstring ExpandTabs(const std::wstring& text);   // 탭을 공백으로 확장
    int GetLineWidth(int lineIndex);                     // 라인 너비 계산
    int GetMenuAreaWidth();                      // 라인 번호 영역 너비 계산
    void DrawSegment(int lineIndex, size_t segStartIdx, const std::wstring& segment, int xOffset, int y, bool isOld);  // 텍스트 세그먼트 그리기
    void HandleTripleClick(POINT point);                // 트리플 클릭 처리
    bool IsWordDelimiter(wchar_t ch);                    // 단어 구분자 여부 확인
    void FindWordBoundary(const std::wstring& text, int position, int& start, int& end);  // 단어 경계 찾기
    void Copy();                                         // 선택 영역 복사

    // 메시지 처리 관련 내부 함수
    void OnPaint();
    void OnSize(int width, int height);
    void OnLButtonDown(UINT nFlags, POINT point);
    void OnLButtonUp(UINT nFlags, POINT point);
    void OnLButtonDblClk(UINT nFlags, POINT point);
    void OnMouseMove(UINT nFlags, POINT point);
    void OnMouseWheel(int zDelta);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    // 구분자 처리용 배열
    bool m_isDivChar[256] = { 0 };
};