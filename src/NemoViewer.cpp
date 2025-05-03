//﻿*******************************************************************************
//    파     일     명 : NemoViewer.cpp
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
#include "NemoViewer.h"
#include <windowsx.h>
#include <sstream>
#include "Util.h"

// COLORREF를 D2D1::ColorF로 변환하는 헬퍼 함수
//inline D2D1::ColorF ColorRefToColorF(COLORREF color) {
//    return D2D1::ColorF(
//        GetRValue(color) / 255.0f,
//        GetGValue(color) / 255.0f,
//        GetBValue(color) / 255.0f,
//        1.0f
//    );
//}
inline D2D1::ColorF ColorRefToColorF(COLORREF color, float opacity = 1.0f) {
    return D2D1::ColorF(
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        opacity  // 알파 채널 설정
    );
}

// NemoViewer 생성자
NemoViewer::NemoViewer()
    : m_hwnd(NULL),
    m_wordWrapWidth(0),
    m_lineSpacing(5),
    m_lineHeight(0),
    m_charWidth(0),
    m_scrollYLine(0),
    m_scrollYWrapLine(0),
    m_menuAreaWidth(0),
    m_lastClickTime(0),
    m_clickCount(0),
    m_pMenuNormalBitmap(nullptr),
    m_pMenuOverBitmap(nullptr),
    m_bStopMenu(false),
    m_isShow(true)
{
    // 여백 초기화
    m_margin = { 5, 15, 5, 0 };

    // 캐럿 위치 초기화
    m_caretPos = TextPos(0, 0);

    // 선택 영역 초기화
    m_selectInfo.start = m_selectInfo.end = m_selectInfo.anchor = m_caretPos;
    m_selectInfo.isSelected = false;
    m_selectInfo.isSelecting = false;

    // 색상 초기화
    m_colorInfo.text = RGB(250, 250, 250);
    m_colorInfo.textBg = RGB(28, 29, 22);
    m_colorInfo.oldText = RGB(140, 140, 140);
    m_colorInfo.select = RGB(0, 102, 204);

    // 빈 라인 추가
    m_lines.push_back(L"");

    // 단어 구분자 초기화
    memset(m_isDivChar, 0, sizeof(m_isDivChar));

    // ASCII 범위 내의 문자들에 대한 구분자 설정
    // 공백 문자
    m_isDivChar[' '] = true;
    m_isDivChar['\t'] = true;
    m_isDivChar['\r'] = true;
    m_isDivChar['\n'] = true;

    // 구두점
    m_isDivChar['.'] = true;
    m_isDivChar[','] = true;
    m_isDivChar[';'] = true;
    m_isDivChar[':'] = true;
    m_isDivChar['!'] = true;
    m_isDivChar['?'] = true;
    m_isDivChar['\''] = true;
    m_isDivChar['"'] = true;
    m_isDivChar['`'] = true;

    // 괄호
    m_isDivChar['('] = true;
    m_isDivChar[')'] = true;
    m_isDivChar['['] = true;
    m_isDivChar[']'] = true;
    m_isDivChar['{'] = true;
    m_isDivChar['}'] = true;
    m_isDivChar['<'] = true;
    m_isDivChar['>'] = true;

    // 연산자 및 기타 특수문자
    m_isDivChar['+'] = true;
    m_isDivChar['-'] = true;
    m_isDivChar['*'] = true;
    m_isDivChar['/'] = true;
    m_isDivChar['='] = true;
    m_isDivChar['&'] = true;
    m_isDivChar['|'] = true;
    m_isDivChar['^'] = true;
    m_isDivChar['~'] = true;
    m_isDivChar['@'] = true;
    m_isDivChar['#'] = true;
    m_isDivChar['$'] = true;
    m_isDivChar['%'] = true;
    m_isDivChar['\\'] = true;
}

// 소멸자
NemoViewer::~NemoViewer() {
    Shutdown();
}

// 초기화
bool NemoViewer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    // Direct2D 렌더러 초기화
    if (!m_d2Render.Initialize(hwnd)) {
        return false;
    }

    // 기본 폰트 설정 (D2Coding 또는 Consolas)
    m_d2Render.SetFont(L"D2Coding", 16, false, false);

    // 색상 설정
    m_d2Render.SetTextColor(m_colorInfo.text);
    m_d2Render.SetOldTextColor(m_colorInfo.oldText);
    m_d2Render.SetBgColor(m_colorInfo.textBg);
    m_d2Render.SetSelectionColors(m_colorInfo.text, m_colorInfo.select);

    // 텍스트 메트릭스 관련 계산
    m_lineHeight = m_d2Render.GetLineHeight() + m_lineSpacing;
    m_charWidth = m_d2Render.GetTextWidth(L"080") - m_d2Render.GetTextWidth(L"08");

    // DWM 프레임 확장 (Windows Vista 이상)
    m_d2Render.ExtendFrameIntoClient();

    // 윈도우 크기 얻기
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    // 렌더러 크기 설정
    m_d2Render.Resize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    // 워드랩 너비 설정
    m_wordWrapWidth = clientRect.right - clientRect.left - GetMenuAreaWidth() - m_margin.right - m_margin.left;

    // 메뉴 이미지 로드 (Direct2D 방식만 사용)
    if (!LoadMenuImages()) {
        // 이미지 로드 실패 시 처리
        TRACE(L"메뉴 이미지 로딩 실패\n");
        // 실패해도 계속 진행 (메뉴 없이도 기본 기능은 동작)
    }

    return true;
}

// 종료
void NemoViewer::Shutdown() {
    // 텍스트 데이터 정리
    m_lines.clear();

    // Direct2D 렌더러 종료
    m_d2Render.Shutdown();
}

void NemoViewer::Show() {
    m_isShow = true;
}

void NemoViewer::Hide() {
    m_isShow = false;
}

// 폰트 설정
void NemoViewer::SetFont(const std::wstring& fontName, int fontSize, bool bold, bool italic) {
    m_d2Render.SetFont(fontName, fontSize, bold, italic);

    // 텍스트 메트릭스 관련 값 업데이트
    m_lineHeight = m_d2Render.GetLineHeight() + m_lineSpacing;
    m_charWidth = m_d2Render.GetTextWidth(L"080") - m_d2Render.GetTextWidth(L"08");

    // 스크롤 사이즈 재계산
    RecalcScrollSizes();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 현재 폰트 정보 얻기
void NemoViewer::GetFont(std::wstring& fontName, int& fontSize, bool& bold, bool& italic) {
    m_d2Render.GetFont(fontName, fontSize, bold, italic);
}

// 폰트 크기 설정
void NemoViewer::SetFontSize(int size) {
    m_d2Render.SetFontSize(size);

    // 텍스트 메트릭스 관련 값 업데이트
    m_lineHeight = m_d2Render.GetLineHeight() + m_lineSpacing;
    m_charWidth = m_d2Render.GetTextWidth(L"080") - m_d2Render.GetTextWidth(L"08");

    // 스크롤 사이즈 재계산
    RecalcScrollSizes();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 현재 폰트 크기 얻기
int NemoViewer::GetFontSize() {
    return m_d2Render.GetFontSize();
}

// 추가 줄 간격 설정
void NemoViewer::SetLineSpacing(int spacing) {
    m_lineSpacing = spacing;
    m_lineHeight = m_d2Render.GetLineHeight() + m_lineSpacing;

    // 스크롤 사이즈 재계산
    RecalcScrollSizes();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 여백 설정
void NemoViewer::SetMargin(int left, int right, int top, int bottom) {
    m_margin.left = left;
    m_margin.right = right;
    m_margin.top = top;
    m_margin.bottom = bottom;

    // 워드랩 너비 재계산
    RECT client;
    GetClientRect(m_hwnd, &client);
    m_wordWrapWidth = client.right - client.left - GetMenuAreaWidth() - m_margin.right - m_margin.left;

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 텍스트 및 배경 색상 설정
void NemoViewer::SetTextColor(COLORREF textColor, COLORREF bgColor) {
    m_colorInfo.text = textColor;
    m_colorInfo.textBg = bgColor;
    m_d2Render.SetTextColor(m_colorInfo.text);
    m_d2Render.SetBgColor(m_colorInfo.textBg);

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 텍스트 색상만 설정
void NemoViewer::SetTextColor(COLORREF textColor) {
    m_colorInfo.text = textColor;
    m_d2Render.SetTextColor(m_colorInfo.text);

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void NemoViewer::SetOldTextColor(COLORREF textColor) {
	m_colorInfo.oldText = textColor;
	m_d2Render.SetOldTextColor(m_colorInfo.oldText);
	// 윈도우 갱신
	InvalidateRect(m_hwnd, NULL, FALSE);
}

// 배경 색상만 설정
void NemoViewer::SetBgColor(COLORREF textBgColor) {
    m_colorInfo.textBg = textBgColor;
    m_d2Render.SetBgColor(m_colorInfo.textBg);

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 텍스트 색상 얻기
COLORREF NemoViewer::GetTextColor() {
    return m_colorInfo.text;
}

// 배경 색상 얻기
COLORREF NemoViewer::GetTextBgColor() {
    return m_colorInfo.textBg;
}

// 라인 번호 색상 설정
void NemoViewer::SetLineNumColor(COLORREF lineNumColor, COLORREF bgColor) {
    m_colorInfo.oldText = lineNumColor;
    m_d2Render.SetOldTextColor(m_colorInfo.oldText);
    
    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 텍스트 설정
void NemoViewer::SetText(const std::wstring& text) {
    // 기존 텍스트 지우기
    Clear();

    // 개행 기준으로 문자열 파싱
    std::vector<std::wstring> lines;
    SplitTextByNewlines(text, lines);

    // 벡터가 비었으면 빈 라인 추가
    if (lines.empty()) {
        m_lines.push_back(L"");
    }
    else {
        // 분할된 라인 추가
        m_lines = std::move(lines);
    }

    // 스크롤 위치 초기화
    m_scrollYLine = 0;
    m_scrollYWrapLine = 0;

    // 캐럿 위치 초기화
    m_caretPos = TextPos(0, 0);
    m_selectInfo.start = m_selectInfo.end = m_selectInfo.anchor = m_caretPos;
    m_selectInfo.isSelected = false;
    m_selectInfo.isSelecting = false;

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    // 스크롤 사이즈 재계산
    RecalcScrollSizes();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void NemoViewer::AddText(std::wstring text) {
    // 개행 기준으로 문자열 파싱
    std::vector<std::wstring> lines;
    SplitTextByNewlines(text, lines);

    // 벡터가 비었으면 빈 라인 추가
    if (lines.empty()) {
        return;
    }
    else {
        // 분할된 라인 추가
        for (int i = 0; i < lines.size(); ++i) {
            m_lines.emplace_back(lines[i]);
            //m_lines = std::move(lines);
        }
    }
    

    // 캐럿을 텍스트 끝으로 이동
    int lastLineIdx = static_cast<int>(m_lines.size() - 1);
    int lastColIdx = static_cast<int>(m_lines[lastLineIdx].length());
    m_caretPos = TextPos(lastLineIdx, lastColIdx);

    // 선택 영역 초기화
    //ClearSelection();

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 전체 텍스트 가져오기
std::wstring NemoViewer::GetText() {
    std::wstring result;

    // 모든 라인을 개행으로 합치기
    for (size_t i = 0; i < m_lines.size(); ++i) {
        result += m_lines[i];
        if (i < m_lines.size() - 1) {
            result += L"\n";
        }
    }

    return result;
}

// 선택된 텍스트 가져오기
std::wstring NemoViewer::GetSelectedText() {
    if (!m_selectInfo.isSelected) {
        return L"";
    }

    // 선택 영역 정규화
    TextPos start = m_selectInfo.start;
    TextPos end = m_selectInfo.end;

    // 선택 영역이 뒤집혔으면 정렬
    if (end.lineIndex < start.lineIndex ||
        (end.lineIndex == start.lineIndex && end.column < start.column)) {
        std::swap(start, end);
    }

    std::wstring result;

    // 동일 라인 내 선택
    if (start.lineIndex == end.lineIndex) {
        if (start.column <= m_lines[start.lineIndex].length()) {
            result = m_lines[start.lineIndex].substr(
                start.column,
                end.column - start.column
            );
        }
    }
    else {
        // 첫 번째 라인의 선택 부분
        if (start.column <= m_lines[start.lineIndex].length()) {
            result = m_lines[start.lineIndex].substr(start.column);
        }

        // 중간 라인들
        for (int i = start.lineIndex + 1; i < end.lineIndex; ++i) {
            result += L"\n" + m_lines[i];
        }

        // 마지막 라인의 선택 부분
        if (end.lineIndex < m_lines.size()) {
            result += L"\n" + m_lines[end.lineIndex].substr(0, end.column);
        }
    }

    return result;
}

// 텍스트 지우기
void NemoViewer::Clear() {
    m_lines.clear();
    m_lines.push_back(L"");

    m_caretPos = TextPos(0, 0);
    m_scrollYLine = 0;
    m_scrollYWrapLine = 0;

    m_selectInfo.start = m_selectInfo.end = m_selectInfo.anchor = m_caretPos;
    m_selectInfo.isSelected = false;
    m_selectInfo.isSelecting = false;
}

// 라인 수 가져오기
size_t NemoViewer::GetLineCount() const {
    return m_lines.size();
}

// 특정 라인으로 이동
void NemoViewer::GotoLine(size_t lineNo) {
    if (lineNo >= m_lines.size()) {
        lineNo = m_lines.size() - 1;
    }

    m_caretPos.lineIndex = static_cast<int>(lineNo);
    m_caretPos.column = 0;

    // 선택 영역 초기화
    ClearSelection();

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    // 윈도우 갱신f
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 모두 선택
void NemoViewer::SelectAll() {
    if (m_lines.empty()) {
        return;
    }

    m_selectInfo.start = TextPos(0, 0);

    int lastLineIdx = static_cast<int>(m_lines.size() - 1);
    int lastColIdx = static_cast<int>(m_lines[lastLineIdx].length());

    m_selectInfo.end = TextPos(lastLineIdx, lastColIdx);
    m_selectInfo.anchor = m_selectInfo.start;
    m_selectInfo.isSelected = true;

    // 캐럿을 선택 영역 끝으로 이동
    m_caretPos = m_selectInfo.end;

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 선택 영역 지우기
void NemoViewer::ClearSelection() {
    m_selectInfo.isSelected = false;
    m_selectInfo.isSelecting = false;
    m_selectInfo.start = m_selectInfo.end = m_selectInfo.anchor = m_caretPos;

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 선택한 텍스트 복사
void NemoViewer::Copy() {
    if (!m_selectInfo.isSelected) {
        return;
    }

    std::wstring selectedText = GetSelectedText();
    if (selectedText.empty()) {
        return;
    }

    // 클립보드 열기
    if (!OpenClipboard(m_hwnd)) {
        return;
    }

    // 클립보드 비우기
    EmptyClipboard();

    // 메모리 할당
    SIZE_T size = (selectedText.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);

    if (hMem) {
        // 메모리에 텍스트 복사
        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));

        if (pMem) {
            wcscpy_s(pMem, selectedText.size() + 1, selectedText.c_str());
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        else {
            GlobalFree(hMem);
        }
    }

    // 클립보드 닫기
    CloseClipboard();
}

// 메시지 처리
LRESULT NemoViewer::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_LBUTTONDOWN:
        OnLButtonDown(static_cast<UINT>(wParam), { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp(static_cast<UINT>(wParam), { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;

    case WM_LBUTTONDBLCLK:
        OnLButtonDblClk(static_cast<UINT>(wParam), { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(static_cast<UINT>(wParam), { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;

    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_KEYDOWN:
        OnKeyDown(static_cast<UINT>(wParam), 1, 0);
        return 0;

    }

    return DefWindowProc(m_hwnd, message, wParam, lParam);
}

// 윈도우 그리기
void NemoViewer::OnPaint() {
    if (!m_isShow) return;

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    Paint(hdc);

    EndPaint(m_hwnd, &ps);
}

// 그리기 함수
void NemoViewer::Paint(HDC hdc) {
    if (!m_isShow) return;
    m_d2Render.BeginDraw();

    // 합성 모드 설정
    ID2D1DeviceContext* pDeviceContext = nullptr;
    m_d2Render.GetRenderTarget()->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&pDeviceContext);
    if (pDeviceContext) {
        // SOURCE_OVER 블렌드 모드 설정 - 알파 블렌딩을 위해 필요
        pDeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        pDeviceContext->Release();
    }

    // 클라이언트 영역 얻기
    RECT client;
    GetClientRect(m_hwnd, &client);

    // 배경을 먼저 완전 투명하게 클리어
    m_d2Render.Clear(m_colorInfo.textBg);

    // 메뉴 이미지 그리기 (Direct2D 방식만 사용)
    if (m_menuAreaWidth > 0) {
        if (!m_bStopMenu) {
            if (m_pMenuNormalBitmap) {
                D2D1_SIZE_F size = m_pMenuNormalBitmap->GetSize();
                m_d2Render.DrawImage(m_pMenuNormalBitmap, 0, 0, size.width, size.height);
            }
        }
        else {
            if (m_pMenuOverBitmap) {
                D2D1_SIZE_F size = m_pMenuOverBitmap->GetSize();
                m_d2Render.DrawImage(m_pMenuOverBitmap, 0, 0, size.width, size.height);
            }
        }
    }

    // 메뉴 영역
    int menuAreaWidth = GetMenuAreaWidth();
    
    // 텍스트 출력 시작 y 좌표
    int y = m_margin.top;

    // 워드랩 모드에서의 그리기
    int lineIndex = m_scrollYLine;
    int wrapLineIndex = m_scrollYWrapLine;

    while (y < client.bottom && lineIndex < static_cast<int>(m_lines.size())) {
        const std::wstring& lineStr = m_lines[lineIndex];

        // 워드랩 위치 계산
        std::vector<int> wrapPositions = FindWordWrapPosition(lineIndex);

        // 워드랩이 없는 경우
        if (wrapPositions.empty()) {
            // 라인 텍스트 그리기
            DrawSegment(lineIndex, 0, lineStr, menuAreaWidth, y, lineIndex!=m_lines.size()-1);

            y += m_lineHeight;
            lineIndex++;
            continue;
        }

        // 워드랩된 각 세그먼트 그리기
        int startPos = 0;
        int skipLineCnt = 0;

        if (lineIndex == m_scrollYLine) {
            // 첫 줄일 경우 시작 위치를 wrapLineIndex로 설정
            skipLineCnt = wrapLineIndex;
        }

        for (size_t i = 0; i < wrapPositions.size() + 1; i++) {
            if (skipLineCnt > 0) {
                skipLineCnt--;

                if (i > 0 && i == wrapPositions.size()) {
                    // 마지막 라인 처리
                    startPos = wrapPositions[i - 1];
                }
                else if (i < wrapPositions.size()) {
                    startPos = wrapPositions[i];
                }

                continue;
            }

            // 화면 밖으로 나가면 중단
            if (y >= client.bottom) {
                break;
            }

            // 워드랩 라인이 보이는 영역에 있는지 확인
            if (lineIndex >= m_scrollYLine) {
                int endPos;

                if (i == wrapPositions.size()) {
                    // 마지막 라인 처리
                    endPos = static_cast<int>(lineStr.length());
                }
                else {
                    endPos = wrapPositions[i];
                }

                // 세그먼트 텍스트 추출
                std::wstring segment = lineStr.substr(startPos, endPos - startPos);

                // 세그먼트 그리기
                DrawSegment(lineIndex, startPos, segment, menuAreaWidth, y, lineIndex != m_lines.size() - 1);

                y += m_lineHeight;
            }

            if (i >= wrapPositions.size()) {
                break;
            }

            startPos = wrapPositions[i];
        }

        lineIndex++;
    }
    m_d2Render.EndDraw();
}

// 윈도우 크기 변경
void NemoViewer::OnSize(int width, int height) {
    // 렌더러 크기 업데이트
    m_d2Render.Resize(width, height);

    // 워드랩 너비 업데이트
    m_wordWrapWidth = width - GetMenuAreaWidth() - m_margin.right - m_margin.left;

    // 스크롤 크기 재계산
    RecalcScrollSizes();

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    m_d2Render.ExtendFrameIntoClient();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 크기 변경
void NemoViewer::Resize(int width, int height) {
    OnSize(width, height);
}

void NemoViewer::SetMenuState(bool bStopMenu) {
    m_bStopMenu = bStopMenu;
    // 화면 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 이미지 로드 메서드 구현
bool NemoViewer::LoadMenuImages() {
    // Direct2D 비트맵 로드
    m_pMenuNormalBitmap = m_d2Render.LoadImageFromFile(L"lmenu_normal.png");
    m_pMenuOverBitmap = m_d2Render.LoadImageFromFile(L"lmenu_over.png");

    if (!m_pMenuNormalBitmap || !m_pMenuOverBitmap) {
        TRACE(L"메뉴 이미지 로딩 실패\n");
        return false;
    }

    return true;
}

void NemoViewer::SetDwmExtend() {
    m_d2Render.ExtendFrameIntoClient();
}

void NemoViewer::ResetDwmExtend() {
    m_d2Render.ResetFrameIntoClient();
}

// 마우스 왼쪽 버튼 눌림
void NemoViewer::OnLButtonDown(UINT nFlags, POINT point) {
    // 현재 클릭 시간 가져오기
    DWORD currentClickTime = GetTickCount();

    // 이전 클릭과 시간 간격(500ms) 및 위치(5픽셀) 확인
    bool isDoubleClickTime = (currentClickTime - m_lastClickTime) < 500;
    bool isNearLastClick = abs(point.x - m_lastClickPos.x) < 5 && abs(point.y - m_lastClickPos.y) < 5;

    // 이전 클릭과 같은 위치에서 빠르게 클릭된 경우
    if (isDoubleClickTime && isNearLastClick) {
        m_clickCount++;

        // 트리플 클릭 확인 (클릭 카운트가 3)
        if (m_clickCount == 3) {
            m_clickCount = 0;  // 카운트 초기화
            HandleTripleClick(point);
            m_lastClickTime = currentClickTime;
            m_lastClickPos = point;
            return;
        }
        else if (m_clickCount == 2) {
            OnLButtonDblClk(nFlags, point);
            m_lastClickTime = currentClickTime;
            m_lastClickPos = point;
            return;
        }
    }
    else {
        // 새로운 클릭 시작
        m_clickCount = 1;
    }

    // 마지막 클릭 정보 저장
    m_lastClickTime = currentClickTime;
    m_lastClickPos = point;

    // 텍스트 위치 계산
    TextPos pos = GetTextPosFromPoint(point);

    // 캐럿 위치 갱신
    m_caretPos.lineIndex = pos.lineIndex;
    m_caretPos.column = pos.column;

    // Shift가 눌리지 않았으면 새로운 선택 시작
    if (!(nFlags & MK_SHIFT)) {
        m_selectInfo.anchor = m_caretPos;
        m_selectInfo.start = m_selectInfo.end = m_caretPos;
        m_selectInfo.isSelected = false;  // 새 선택 시작 시 초기화
    }
    else {
        // Shift가 눌린 상태에서 클릭: 기존 선택 확장 또는 새 선택 시작
        if (!m_selectInfo.isSelected) {
            // 이전에 선택 영역이 없었다면 현재 앵커를 유지
            m_selectInfo.anchor = m_selectInfo.start;
        }

        // 선택 영역 업데이트 (앵커부터 현재 캐럿까지)
        if (m_selectInfo.anchor.lineIndex < m_caretPos.lineIndex ||
            (m_selectInfo.anchor.lineIndex == m_caretPos.lineIndex &&
                m_selectInfo.anchor.column < m_caretPos.column)) {
            m_selectInfo.start = m_selectInfo.anchor;
            m_selectInfo.end = m_caretPos;
        }
        else {
            m_selectInfo.start = m_caretPos;
            m_selectInfo.end = m_selectInfo.anchor;
        }

        // 선택 영역 설정
        m_selectInfo.isSelected = true;
    }

    m_selectInfo.isSelecting = true;
    SetCapture(m_hwnd);

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 마우스 왼쪽 버튼 놓음
void NemoViewer::OnLButtonUp(UINT nFlags, POINT point) {
    if (m_selectInfo.isSelecting) {
        ReleaseCapture();
        m_selectInfo.isSelecting = false;
    }
}

// 마우스 더블클릭
void NemoViewer::OnLButtonDblClk(UINT nFlags, POINT point) {
    // 클릭 카운트를 2로 설정 (더블 클릭)
    m_clickCount = 2;

    // 마지막 클릭 정보 업데이트
    m_lastClickTime = GetTickCount();
    m_lastClickPos = point;

    // 텍스트 위치 계산
    TextPos pos = GetTextPosFromPoint(point);

    if (pos.lineIndex >= static_cast<int>(m_lines.size())) {
        return;
    }

    // 현재 라인의 텍스트 가져오기
    const std::wstring& lineText = m_lines[pos.lineIndex];

    // 단어 경계 찾기
    int wordStart, wordEnd;
    FindWordBoundary(lineText, pos.column, wordStart, wordEnd);

    // 선택된 내용이 없으면 중단
    if (wordStart == wordEnd) {
        return;
    }

    // 선택 영역 설정
    m_selectInfo.start.lineIndex = pos.lineIndex;
    m_selectInfo.start.column = wordStart;
    m_selectInfo.end.lineIndex = pos.lineIndex;
    m_selectInfo.end.column = wordEnd;
    m_selectInfo.isSelected = true;
    m_selectInfo.anchor = m_selectInfo.start;

    // 캐럿은 단어의 끝에 위치
    m_caretPos.lineIndex = pos.lineIndex;
    m_caretPos.column = wordEnd;

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 마우스 이동
void NemoViewer::OnMouseMove(UINT nFlags, POINT point) {
    if (m_selectInfo.isSelecting && (nFlags & MK_LBUTTON)) {
        // 영역 밖으로 드래그 시 자동 스크롤
        RECT client;
        GetClientRect(m_hwnd, &client);

        if (point.y < 0) {
            if (m_scrollYLine > 0) {
                m_scrollYLine--;
                m_scrollYWrapLine = 0;
            }
        }
        else if (point.y > client.bottom) {
            if (m_scrollYLine < static_cast<int>(m_lines.size()) - 1) {
                m_scrollYLine++;
                m_scrollYWrapLine = 0;
            }
        }

        // 텍스트 위치 계산
        TextPos newPos = GetTextPosFromPoint(point);

        if (newPos.lineIndex != m_caretPos.lineIndex || newPos.column != m_caretPos.column) {
            m_caretPos = newPos;

            // 선택 영역 업데이트
            if (m_selectInfo.anchor.lineIndex < m_caretPos.lineIndex ||
                (m_selectInfo.anchor.lineIndex == m_caretPos.lineIndex &&
                    m_selectInfo.anchor.column < m_caretPos.column)) {
                m_selectInfo.start = m_selectInfo.anchor;
                m_selectInfo.end = m_caretPos;
            }
            else {
                m_selectInfo.start = m_caretPos;
                m_selectInfo.end = m_selectInfo.anchor;
            }

            // 앵커와 캐럿 위치가 다르면 선택 영역 있음
            m_selectInfo.isSelected = !(m_selectInfo.anchor.lineIndex == m_caretPos.lineIndex &&
                m_selectInfo.anchor.column == m_caretPos.column);

            // 윈도우 갱신
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
}

// 마우스 휠 스크롤
void NemoViewer::OnMouseWheel(int zDelta) {
    // 기본 휠 스크롤 라인 수 설정
    int linesToScroll = 3;

    if (zDelta > 0) {
        // 위로 스크롤
        if (m_scrollYLine > 0) {
            m_scrollYLine = max(0, m_scrollYLine - linesToScroll);
            m_scrollYWrapLine = 0;
        }
    }
    else {
        // 아래로 스크롤
        int maxLines = static_cast<int>(m_lines.size());
        m_scrollYLine = min(maxLines - 1, m_scrollYLine + linesToScroll);
        m_scrollYWrapLine = 0;
    }

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 키보드 이벤트 처리
void NemoViewer::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    // Ctrl 키 상태 확인
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    // Shift 키 상태 확인
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    // Ctrl+C: 복사
    if (ctrl && nChar == 'C') {
        Copy();
        return;
    }

    // Ctrl+A: 전체 선택
    if (ctrl && nChar == 'A') {
        SelectAll();
        return;
    }

    // ESC: 선택 취소
    if (nChar == VK_ESCAPE) {
        if (m_selectInfo.isSelected) {
            ClearSelection();
        }
        return;
    }

    // 화살표 키, Home, End, Page Up, Page Down 등 네비게이션 키 처리
    switch (nChar) {
    case VK_LEFT:
        if (m_caretPos.column > 0) {
            m_caretPos.column--;
        }
        else if (m_caretPos.lineIndex > 0) {
            m_caretPos.lineIndex--;
            m_caretPos.column = static_cast<int>(m_lines[m_caretPos.lineIndex].length());
        }
        break;

    case VK_RIGHT:
        if (m_caretPos.column < static_cast<int>(m_lines[m_caretPos.lineIndex].length())) {
            m_caretPos.column++;
        }
        else if (m_caretPos.lineIndex < static_cast<int>(m_lines.size()) - 1) {
            m_caretPos.lineIndex++;
            m_caretPos.column = 0;
        }
        break;

    case VK_UP:
        if (m_caretPos.lineIndex > 0) {
            // 워드랩 라인 계산
            std::vector<int> wrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);
            int currentWrapLine = 0;
            int wrapStartCol = 0;

            // 현재 캐럿 위치가 어느 워드랩 라인에 속하는지 확인
            for (size_t i = 0; i < wrapPositions.size(); i++) {
                if (m_caretPos.column < wrapPositions[i]) {
                    break;
                }
                currentWrapLine++;
                wrapStartCol = wrapPositions[i];
            }

            // 현재 라인 내에서 이전 워드랩 라인으로 이동
            if (currentWrapLine > 0) {
                int prevWrapStartCol = (currentWrapLine > 1) ? wrapPositions[currentWrapLine - 2] : 0;
                int relativeCol = m_caretPos.column - wrapStartCol;
                m_caretPos.column = prevWrapStartCol + min(relativeCol, wrapPositions[currentWrapLine - 1] - prevWrapStartCol);
            }
            else {
                // 이전 라인으로 이동
                m_caretPos.lineIndex--;

                // 이전 라인의 워드랩 정보 계산
                std::vector<int> prevWrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);

                // 상대적인 같은 열 위치 유지 시도
                int relativeCol = m_caretPos.column - wrapStartCol;

                if (prevWrapPositions.empty()) {
                    // 이전 라인에 워드랩이 없으면
                    m_caretPos.column = min(relativeCol, static_cast<int>(m_lines[m_caretPos.lineIndex].length()));
                }
                else {
                    // 이전 라인의 마지막 워드랩 라인으로 이동
                    int lastWrapStartCol = prevWrapPositions.back();
                    m_caretPos.column = lastWrapStartCol + min(relativeCol,
                        static_cast<int>(m_lines[m_caretPos.lineIndex].length()) - lastWrapStartCol);
                }
            }
        }
        break;

    case VK_DOWN:
        if (m_caretPos.lineIndex < static_cast<int>(m_lines.size()) - 1) {
            // 워드랩 라인 계산
            std::vector<int> wrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);
            int currentWrapLine = 0;
            int wrapStartCol = 0;

            // 현재 캐럿 위치가 어느 워드랩 라인에 속하는지 확인
            for (size_t i = 0; i < wrapPositions.size(); i++) {
                if (m_caretPos.column < wrapPositions[i]) {
                    break;
                }
                currentWrapLine++;
                wrapStartCol = wrapPositions[i];
            }

            // 현재 라인 내에서 다음 워드랩 라인으로 이동
            if (currentWrapLine < static_cast<int>(wrapPositions.size())) {
                int nextWrapStartCol = wrapPositions[currentWrapLine];
                int nextWrapEndCol = (currentWrapLine + 1 < wrapPositions.size()) ?
                    wrapPositions[currentWrapLine + 1] : static_cast<int>(m_lines[m_caretPos.lineIndex].length());

                int relativeCol = m_caretPos.column - wrapStartCol;
                m_caretPos.column = nextWrapStartCol + min(relativeCol, nextWrapEndCol - nextWrapStartCol);
            }
            else {
                // 다음 라인으로 이동
                m_caretPos.lineIndex++;

                // 다음 라인의 워드랩 정보 계산
                std::vector<int> nextWrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);

                // 상대적인 같은 열 위치 유지 시도
                int relativeCol = m_caretPos.column - wrapStartCol;

                if (nextWrapPositions.empty() || nextWrapPositions.size() == 0) {
                    // 다음 라인에 워드랩이 없으면
                    m_caretPos.column = min(relativeCol, static_cast<int>(m_lines[m_caretPos.lineIndex].length()));
                }
                else {
                    // 다음 라인의 첫 번째 워드랩 라인으로 이동
                    m_caretPos.column = min(relativeCol, nextWrapPositions[0]);
                }
            }
        }
        break;

    case VK_HOME:
        if (ctrl) {
            // Ctrl+Home: 문서 시작으로 이동
            m_caretPos.lineIndex = 0;
            m_caretPos.column = 0;
        }
        else {
            // 현재 라인의 시작으로 이동 (워드랩 고려)
            std::vector<int> wrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);
            int wrapStartCol = 0;

            for (size_t i = 0; i < wrapPositions.size(); i++) {
                if (m_caretPos.column < wrapPositions[i]) {
                    break;
                }
                wrapStartCol = wrapPositions[i];
            }

            m_caretPos.column = wrapStartCol;
        }
        break;

    case VK_END:
        if (ctrl) {
            // Ctrl+End: 문서 끝으로 이동
            m_caretPos.lineIndex = static_cast<int>(m_lines.size()) - 1;
            m_caretPos.column = static_cast<int>(m_lines[m_caretPos.lineIndex].length());
        }
        else {
            // 현재 라인의 끝으로 이동 (워드랩 고려)
            std::vector<int> wrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);
            int currentWrapLine = 0;
            int wrapStartCol = 0;

            for (size_t i = 0; i < wrapPositions.size(); i++) {
                if (m_caretPos.column < wrapPositions[i]) {
                    break;
                }
                currentWrapLine++;
                wrapStartCol = wrapPositions[i];
            }

            if (currentWrapLine < static_cast<int>(wrapPositions.size())) {
                m_caretPos.column = wrapPositions[currentWrapLine] - 1;
            }
            else {
                m_caretPos.column = static_cast<int>(m_lines[m_caretPos.lineIndex].length());
            }
        }
        break;

    case VK_PRIOR:  // Page Up
    {
        RECT client;
        GetClientRect(m_hwnd, &client);
        int visibleLines = (client.bottom - client.top - m_margin.top - m_margin.bottom) / m_lineHeight;

        // 페이지 위로 이동 (화면에 표시되는 라인 수만큼)
        if (m_scrollYLine > 0) {
            m_scrollYLine = max(0, m_scrollYLine - visibleLines);
            m_scrollYWrapLine = 0;

            // 캐럿도 함께 이동
            int oldColumn = m_caretPos.column;
            m_caretPos.lineIndex = max(0, m_caretPos.lineIndex - visibleLines);

            // 열 위치 조정 (라인 길이를 넘지 않도록)
            m_caretPos.column = min(oldColumn, static_cast<int>(m_lines[m_caretPos.lineIndex].length()));
        }
    }
    break;

    case VK_NEXT:  // Page Down
    {
        RECT client;
        GetClientRect(m_hwnd, &client);
        int visibleLines = (client.bottom - client.top - m_margin.top - m_margin.bottom) / m_lineHeight;

        // 페이지 아래로 이동 (화면에 표시되는 라인 수만큼)
        int maxLines = static_cast<int>(m_lines.size());
        if (m_scrollYLine < maxLines - 1) {
            m_scrollYLine = min(maxLines - 1, m_scrollYLine + visibleLines);
            m_scrollYWrapLine = 0;

            // 캐럿도 함께 이동
            int oldColumn = m_caretPos.column;
            m_caretPos.lineIndex = min(maxLines - 1, m_caretPos.lineIndex + visibleLines);

            // 열 위치 조정 (라인 길이를 넘지 않도록)
            m_caretPos.column = min(oldColumn, static_cast<int>(m_lines[m_caretPos.lineIndex].length()));
        }
    }
    break;
    }

    // Shift 키가 눌린 경우 선택 영역 처리
    if (shift) {
        // 기존에 선택 영역이 없었다면 앵커 설정
        if (!m_selectInfo.isSelected) {
            m_selectInfo.anchor = TextPos(m_caretPos.lineIndex, m_caretPos.column);
        }

        // 선택 영역 업데이트 (앵커 기준)
        if (m_selectInfo.anchor.lineIndex < m_caretPos.lineIndex ||
            (m_selectInfo.anchor.lineIndex == m_caretPos.lineIndex &&
                m_selectInfo.anchor.column < m_caretPos.column)) {
            m_selectInfo.start = m_selectInfo.anchor;
            m_selectInfo.end = m_caretPos;
        }
        else {
            m_selectInfo.start = m_caretPos;
            m_selectInfo.end = m_selectInfo.anchor;
        }

        m_selectInfo.isSelected = true;
    }
    else if(!ctrl) {
        // Shift가 안 눌린 경우 선택 영역 취소
        m_selectInfo.isSelected = false;
        m_selectInfo.start = m_selectInfo.end = m_selectInfo.anchor = m_caretPos;
    }

    // 캐럿 위치 업데이트
    EnsureCaretVisible();

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 트리플 클릭 처리: 현재 라인 전체 선택
void NemoViewer::HandleTripleClick(POINT point) {
    TextPos pos = GetTextPosFromPoint(point);

    if (pos.lineIndex >= static_cast<int>(m_lines.size())) {
        return;
    }

    // 라인 전체 선택
    m_selectInfo.start.lineIndex = pos.lineIndex;
    m_selectInfo.start.column = 0;
    m_selectInfo.end.lineIndex = pos.lineIndex;
    m_selectInfo.end.column = static_cast<int>(m_lines[pos.lineIndex].length());
    m_selectInfo.isSelected = true;
    m_selectInfo.anchor = m_selectInfo.start;

    // 캐럿 위치 업데이트
    m_caretPos = m_selectInfo.end;

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// 단어 구분자 여부 확인
bool NemoViewer::IsWordDelimiter(wchar_t ch) {
    // ASCII 범위 내의 문자는 미리 계산된 배열 사용
    if (ch < 256) {
        return m_isDivChar[ch];
    }

    // ASCII 범위 밖의 문자는 공백이나 구두점인지 확인
    return iswspace(ch) || (iswpunct(ch) && ch != L'_');
}

// 단어 경계 찾기
void NemoViewer::FindWordBoundary(const std::wstring& text, int position, int& start, int& end) {
    int length = static_cast<int>(text.length());

    // 빈 문자열이거나 범위를 벗어난 경우
    if (length == 0 || position < 0 || position >= length) {
        start = end = position;
        return;
    }

    wchar_t currentChar = text[position];
    bool isDelimiter = IsWordDelimiter(currentChar);

    // 현재 위치가 단어 구분자인 경우
    if (isDelimiter) {
        // 같은 종류의 구분자 그룹으로 확장
        bool isSpace = iswspace(currentChar);

        // 시작 위치 찾기
        start = position;
        while (start > 0) {
            wchar_t prevChar = text[start - 1];

            // 공백 문자 그룹
            if (isSpace) {
                if (!iswspace(prevChar)) break;
            }
            // 같은 구분자 그룹
            else if (currentChar == prevChar) {
                // 계속 확장
            }
            // 다른 구분자면 중단
            else {
                break;
            }

            start--;
        }

        // 끝 위치 찾기
        end = position;
        while (end < length - 1) {
            wchar_t nextChar = text[end + 1];

            // 공백 문자 그룹
            if (isSpace) {
                if (!iswspace(nextChar)) break;
            }
            // 같은 구분자 그룹
            else if (currentChar == nextChar) {
                // 계속 확장
            }
            // 다른 구분자면 중단
            else {
                break;
            }

            end++;
        }
    }
    // 일반 단어의 경우
    else {
        // 단어 시작 위치 찾기
        start = position;
        while (start > 0 && !IsWordDelimiter(text[start - 1])) {
            start--;
        }

        // 단어 끝 위치 찾기
        end = position;
        while (end < length - 1 && !IsWordDelimiter(text[end + 1])) {
            end++;
        }
    }

    end++; // 끝 위치는 마지막 문자 다음 위치
}

// 텍스트 워드랩 위치 계산
std::vector<int> NemoViewer::FindWordWrapPosition(int lineIndex) {
    std::vector<int> wrapPos;

    if (lineIndex < 0 || lineIndex >= static_cast<int>(m_lines.size())) {
        return wrapPos;
    }

    const std::wstring& lineText = m_lines[lineIndex];
    if (lineText.empty()) {
        return wrapPos;
    }

    // 라인이 한 줄에 다 들어갈 경우
    int lineWidth = GetLineWidth(lineIndex);
    if (lineWidth <= m_wordWrapWidth) {
        return wrapPos;
    }

    int currentPos = 0;
    int currWidth = 0;

    while (currentPos < static_cast<int>(lineText.length())) {
        // 이진 검색으로 현재 위치에서 가장 적절한 텍스트 길이 찾기
        int low = 1;
        int high = static_cast<int>(lineText.length()) - currentPos;
        int result = 1;

        while (low <= high) {
            int mid = (low + high) / 2;
            if (mid <= 0) mid = 1;

            std::wstring testText = ExpandTabs(lineText.substr(currentPos, mid));
            int testWidth = static_cast<int>(m_d2Render.GetTextWidth(testText));

            if (testWidth < m_wordWrapWidth) {
                currWidth = testWidth;
                result = mid;
                low = mid + 1;
            }
            else {
                high = mid - 1;
            }
        }

        currentPos += result;
        wrapPos.push_back(currentPos);

        // 줄이 끝나면 종료
        if (lineWidth - currWidth < m_wordWrapWidth) {
            break;
        }
    }

    return wrapPos;
}

// 캐럿 픽셀 위치 계산
POINT NemoViewer::GetCaretPixelPos(const TextPos& pos) {
    POINT pt = { 0, 0 };

    int lineIndex = pos.lineIndex;
    if (lineIndex < 0) {
        lineIndex = 0;
    }
    if (lineIndex >= static_cast<int>(m_lines.size())) {
        lineIndex = static_cast<int>(m_lines.size()) - 1;
    }

    // 클라이언트 영역 크기 계산
    RECT client;
    GetClientRect(m_hwnd, &client);
    int screenWidth = client.right - client.left - m_margin.left - m_margin.right - GetMenuAreaWidth();
    int screenHeight = client.bottom - client.top - m_margin.top - m_margin.bottom;
    int visibleLines = screenHeight / m_lineHeight;

    // 현재 라인의 몇 번째 워드랩 라인인지 계산
    int wrapLineIndex = 0;
    int startCol = 0;
    std::vector<int> wrapPositions = FindWordWrapPosition(lineIndex);

    for (size_t i = 0; i < wrapPositions.size(); i++) {
        if (pos.column < wrapPositions[i]) {
            break;
        }
        startCol = wrapPositions[i];
        wrapLineIndex++;
    }

    // 화면 위에 있을 경우
    if (lineIndex < m_scrollYLine ||
        (lineIndex == m_scrollYLine && wrapLineIndex < m_scrollYWrapLine)) {
        pt.y = -m_lineHeight;
    }
    // 화면 밑에 있을 경우
    else if (lineIndex > m_scrollYLine + visibleLines) {
        pt.y = screenHeight + m_lineHeight;
    }
    // 화면에 보이는 라인일 경우
    else {
        // 수직 위치 계산
        int totalLines = -m_scrollYWrapLine;

        for (int i = m_scrollYLine; i < lineIndex; i++) {
            totalLines += static_cast<int>(FindWordWrapPosition(i).size()) + 1;
            if (totalLines > visibleLines) {
                break;
            }
        }

        totalLines += wrapLineIndex;

        // 화면 밑에 있을 경우
        if (totalLines >= visibleLines) {
            pt.y = screenHeight + m_lineHeight;
        }
        // 화면 안에 있을 경우
        else {
            pt.y = totalLines * m_lineHeight;
        }
    }

    // 수평 위치 계산
    const std::wstring& line = m_lines[lineIndex];

    if (!line.empty() && pos.column >= startCol) {
        if (pos.column == startCol) {
            pt.x = 0;
        }
        else {
            std::wstring text = line.substr(startCol, pos.column - startCol);
            text = ExpandTabs(text);
            pt.x = static_cast<int>(m_d2Render.GetTextWidth(text));
        }
    }
    else {
        pt.x = 0;
    }

    return pt;
}

// 텍스트 위치 계산
TextPos NemoViewer::GetTextPosFromPoint(POINT point) {
    TextPos pos;

    // 클라이언트 영역 얻기
    RECT client;
    GetClientRect(m_hwnd, &client);
    int visibleLines = (client.bottom - client.top) / m_lineHeight;

    // 수직 위치 계산
    int lineSize = static_cast<int>(m_lines.size());
    int prevY = 0;
    std::vector<int> wrapCols;
    int wrapColsSize = 0;
    int startCol = 0;
    int totalPrevLines = -m_scrollYWrapLine;

    for (int i = m_scrollYLine; i < lineSize; i++) {
        wrapCols = FindWordWrapPosition(i);
        totalPrevLines += static_cast<int>(wrapCols.size()) + 1;
        prevY = totalPrevLines * m_lineHeight + m_margin.top;
        pos.lineIndex = i;

        if (prevY >= point.y) {
            pos.lineIndex = i;

            // wrapLineIndex 계산
            wrapColsSize = static_cast<int>(wrapCols.size());
            prevY -= wrapColsSize * m_lineHeight;

            if (prevY >= point.y) {
                // 현재 줄의 wordwrap 시작 컬럼을 찾는다.
                startCol = 0;
                break;
            }

            for (int j = 0; j < wrapColsSize; j++) {
                if (prevY + m_lineHeight * (j + 1) >= point.y) {
                    // 현재 줄의 wordwrap 시작 컬럼을 찾는다.
                    startCol = wrapCols[j];
                    break;
                }
            }
            break;
        }

        if (totalPrevLines > visibleLines) {
            break;
        }
    }

    // 수평 위치 계산
    if (pos.lineIndex >= 0 && pos.lineIndex < static_cast<int>(m_lines.size())) {
        const std::wstring& lineText = m_lines[pos.lineIndex];
        std::wstring tabText;
        int col = 0;

        // 라인 번호 영역 및 여백 고려
        int pointX = point.x - GetMenuAreaWidth() - m_margin.left;

        if (!lineText.empty()) {
            // 이진 검색으로 텍스트에서 현재 위치 찾기
            int low = 0;
            int high = static_cast<int>(lineText.size()) - startCol;
            int result = 0;

            while (low <= high) {
                int mid = (low + high) / 2;
                if (mid < 0) mid = 0;

                tabText = ExpandTabs(lineText.substr(startCol, mid));
                int testSize = static_cast<int>(m_d2Render.GetTextWidth(tabText));

                if (testSize <= pointX) {
                    // 더 많은 텍스트를 포함할 수 있음
                    result = mid;
                    low = mid + 1;
                }
                else {
                    // 너무 많음 -> 줄이기
                    high = mid - 1;
                }
            }

            col = startCol + result;
        }

        pos.column = col;
    }

    return pos;
}

// 탭을 공백으로 확장
std::wstring NemoViewer::ExpandTabs(const std::wstring& text) {
    if (text.find(L'\t') == std::wstring::npos) {
        return text;  // 탭이 없으면 원본 반환
    }

    std::wstring result;
    result.reserve(text.length() * 2);  // 여유있게 공간 할당

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == L'\t') {
            // 탭 문자를 공백으로 변환 (4칸 고정)
            for (int j = 0; j < 4; ++j) {
                result.push_back(L' ');
            }
        }
        else {
            result.push_back(text[i]);
        }
    }

    return result;
}

// 라인 너비 계산
int NemoViewer::GetLineWidth(int lineIndex) {
    if (lineIndex < 0 || lineIndex >= static_cast<int>(m_lines.size())) {
        return 0;
    }

    std::wstring line = m_lines[lineIndex];
    line = ExpandTabs(line);
	return static_cast<int>(m_d2Render.GetTextWidth(line));
}

// 라인 번호 영역 너비 계산
int NemoViewer::GetMenuAreaWidth() {
    return m_menuAreaWidth;
}

void NemoViewer::SetMenuAreaWidth(int width) {
	m_menuAreaWidth = width;
}

void NemoViewer::SetBgOpacity(float opacity) {
    // 0.0f ~ 1.0f 범위로 제한
    opacity = max(0.0f, min(1.0f, opacity));
    // D2Render에 배경 투명도 설정
    m_d2Render.SetBgOpacity(opacity);

    // 윈도우 갱신
    InvalidateRect(m_hwnd, NULL, FALSE);
}

float NemoViewer::GetBgOpacity() const {
    return m_d2Render.GetBgOpacity();
}

// 텍스트 세그먼트 그리기
void NemoViewer::DrawSegment(int lineIndex, size_t segStartIdx, const std::wstring& segment, int xOffset, int y, bool isOld) {
    if (segment.empty()) {
        // 내용이 없는 경우도 캐럿 표시 위해 배경색으로 칠하기
        D2D1_RECT_F lineRect = D2D1::RectF(
            static_cast<float>(xOffset),
            static_cast<float>(y),
            static_cast<float>(xOffset + 2),
            static_cast<float>(y + m_lineHeight)
        );
        m_d2Render.FillSolidRect(lineRect, m_colorInfo.textBg);
        return;
    }

    std::wstring segText = segment;
    std::wstring tabText = ExpandTabs(segText);

    RECT client;
    GetClientRect(m_hwnd, &client);

    // 수직 클리핑
    if (y + m_lineHeight <= 0 || y >= client.bottom) {
        return;  // 보이지 않는 영역은 출력 생략
    }

    int x = xOffset + m_margin.left;

    // 텍스트 전체 너비 계산
    int textWidth = static_cast<int>(m_d2Render.GetTextWidth(tabText));

    // 수평 클리핑
    if (x + textWidth <= 0 || x >= client.right) {
        return;  // 완전히 화면 밖에 있으면 그리지 않음
    }

    // 이 세그먼트 내 선택 영역 계산
    bool hasSelection = m_selectInfo.isSelected;
    int selStartCol = 0, selEndCol = 0;

    if (hasSelection) {
        // 선택 영역 정규화
        TextPos s = m_selectInfo.start;
        TextPos e = m_selectInfo.end;

        // 선택 영역이 뒤집혔으면 정렬
        if (e.lineIndex < s.lineIndex || (e.lineIndex == s.lineIndex && e.column < s.column)) {
            s = m_selectInfo.end;
            e = m_selectInfo.start;
        }

        // 텍스트에 선택 영역 표시
        if (lineIndex >= s.lineIndex && lineIndex <= e.lineIndex) {
            // 한 라인에만 선택 영역이 존재할 경우
            if (s.lineIndex == e.lineIndex) {
                selStartCol = s.column - static_cast<int>(segStartIdx);
                if (selStartCol < 0) selStartCol = 0;
                selEndCol = e.column - static_cast<int>(segStartIdx);
                if (selEndCol <= selStartCol) selEndCol = selStartCol;
            }
            // 현재 라인이 선택 영역의 시작인 경우
            else if (lineIndex == s.lineIndex) {
                selStartCol = s.column - static_cast<int>(segStartIdx);
                if (selStartCol < 0) selStartCol = 0;
                selEndCol = static_cast<int>(segText.size());
            }
            // 현재 라인이 선택 영역의 끝인 경우
            else if (lineIndex == e.lineIndex) {
                selStartCol = 0;
                selEndCol = e.column - static_cast<int>(segStartIdx);
                if (selEndCol <= selStartCol) selEndCol = selStartCol;
            }
            // 선택 영역 중간에 있는 라인인 경우
            else {
                selStartCol = 0;
                selEndCol = static_cast<int>(segText.size());
            }
        }
    }

    // 클리핑 영역 설정 - 보이는 영역으로 제한
    D2D1_RECT_F clipRect = D2D1::RectF(
        static_cast<float>(max(xOffset, x)),
        static_cast<float>(y),
        static_cast<float>(min(client.right, x + textWidth)),
        static_cast<float>(y + m_lineHeight)
    );

    if (hasSelection && selEndCol > selStartCol) {
        m_d2Render.DrawEditText(x, y, &clipRect, tabText.c_str(), tabText.size(), true, selStartCol, selEndCol, isOld);
    }
    else {
        // 선택 없는 경우 - 클리핑된 영역만 효율적으로 그리기
        m_d2Render.DrawEditText(x, y, &clipRect, tabText.c_str(), tabText.size(), isOld);
    }
}

// 캐럿이 보이도록 스크롤 조정
void NemoViewer::EnsureCaretVisible() {
    POINT pt = GetCaretPixelPos(m_caretPos);

    // 화면 표시 영역 크기 계산
    RECT client;
    GetClientRect(m_hwnd, &client);
    if (client.right - client.left <= 0 || client.bottom - client.top <= 0) {
        return;  // 화면이 없을 경우
    }

    int screenWidth = client.right - client.left - m_margin.left - m_margin.right - GetMenuAreaWidth();
    int screenHeight = client.bottom - client.top - m_margin.top - m_margin.bottom;
    int visibleLines = screenHeight / m_lineHeight;  // 마진을 제외한 화면에 보이는 줄 수

    // 현재 라인의 몇 번째 워드랩 라인인지 계산
    int wrapLineIndex = 0;
    int startCol = 0;
    std::vector<int> wrapPositions = FindWordWrapPosition(m_caretPos.lineIndex);

    for (size_t i = 0; i < wrapPositions.size(); i++) {
        if (m_caretPos.column < wrapPositions[i]) {
            break;
        }
        startCol = wrapPositions[i];
        wrapLineIndex++;
    }

    // 캐럿이 화면 위로 벗어남
    if (pt.y < 0) {
        m_scrollYLine = m_caretPos.lineIndex;
        m_scrollYWrapLine = wrapLineIndex;
        pt.y = 0;
    }
    // 캐럿이 화면 아래로 벗어남: 현재의 캐럿 위치에서 위로 화면 줄수에 맞는 시작점을 구함
    else if (pt.y + m_lineHeight > screenHeight) {
        int totalLineCnt = 0;
        int wrapCnt = 0;
        totalLineCnt = wrapLineIndex + 1;

        for (int i = m_caretPos.lineIndex - 1; i >= 0; i--) {
            wrapCnt = static_cast<int>(FindWordWrapPosition(i).size()) + 1;  // 화면 줄수 + 워드랩 줄수
            totalLineCnt += wrapCnt;
            m_scrollYLine = i;
            m_scrollYWrapLine = 0;

            if (totalLineCnt >= visibleLines) {
                // 초과된 라인: 전체라인 - 화면라인수
                // 마지막 라인의 워드랩 수: 마지막 라인의 워드랩 라인수 - 초과된 라인수
                m_scrollYWrapLine = totalLineCnt - visibleLines;
                break;
            }
        }
        pt.y = (visibleLines - 1) * m_lineHeight;
    }
    else {
        // wrapLineIndex 변경 확인
        if (m_scrollYLine < m_lines.size()) {
            int wrapLineIndex = static_cast<int>(FindWordWrapPosition(m_scrollYLine).size());
            if (m_scrollYWrapLine > wrapLineIndex) {
                m_scrollYWrapLine = wrapLineIndex;
            }
        }
    }

    m_scrollYLine = max(0, m_scrollYLine);
}

// 스크롤 사이즈 재계산
void NemoViewer::RecalcScrollSizes() {
    if (!m_hwnd) {
        return;
    }

    RECT client;
    GetClientRect(m_hwnd, &client);

    // 워드랩 너비 계산
    m_wordWrapWidth = client.right - client.left - GetMenuAreaWidth() - m_margin.right - m_margin.left;
}

// 텍스트를 줄바꿈으로 분리
void NemoViewer::SplitTextByNewlines(const std::wstring& text, std::vector<std::wstring>& parts) {
    parts.clear();

    // 대량 텍스트인 경우 미리 메모리 할당
    if (text.length() > 10000) {
        // 대략적인 라인 수 추정 (평균 라인 길이를 50자로 가정)
        size_t estimatedLines = text.length() / 50 + 100;
        parts.reserve(estimatedLines);
    }

    size_t pos = 0;
    std::wstring line;

    // 문자열 전체를 한 번만 순회
    while (pos < text.length()) {
        // skip CR
        if (text[pos] == L'\r') {
            pos++; // CRLF 처리
            continue;
        }

        if (text[pos] == L'\n') {
            if (!line.empty()) {
                parts.push_back(line);
                line.clear(); // 개행 문자 처리 후 라인 초기화
            }
        }
        else {
            // 개행 문자가 아닌 경우
            line += text[pos];
        }

        pos++;
    }

    // 마지막 라인 처리
    if (!line.empty()) {
        parts.push_back(line);
    }
}