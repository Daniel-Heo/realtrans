//﻿*******************************************************************************
//    파     일     명 : D2Render.h
//    프로그램명칭 : Direct2D 기반 텍스트 렌더링
//    프로그램용도 : Direct2D를 사용하여 텍스트를 렌더링하는 클래스
//    참  고  사  항  : 
//
//    작    성    자 : Original by Daniel Heo
//    라 이  센 스  : MIT License
//    ----------------------------------------------------------------------------
//    수정일자    수정자      수정내용
//    =========== =========== ====================================================
//    2025.4.22   NemoEdit에서 Win32 버전으로 변환
//*******************************************************************************
#pragma once
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <atlbase.h>
#include <string>
#include <vector>
#include <wincodec.h> // WIC를 위한 헤더 추가

#pragma comment(lib, "dwmapi.lib")

#define OLD_LINE false
#define LAST_LINE true

// TextMetrics 구조체 정의
struct TextMetrics {
    float ascent;                    // 기준선에서 문자의 최상단까지의 거리
    float descent;                   // 기준선에서 문자의 최하단까지의 거리
    float lineGap;                   // 줄 간 여백 크기
    float capHeight;                 // 대문자의 높이
    float xHeight;                   // 소문자 'x'의 높이 (소문자 높이 기준)
    float underlinePosition;         // 밑줄의 세로 위치
    float underlineThickness;        // 밑줄의 두께
    float strikethroughPosition;     // 취소선의 세로 위치
    float strikethroughThickness;    // 취소선의 두께
    float lineHeight;                // 한 줄의 전체 높이
    float oldLineHeight;        // 이전 텍스트의 한 줄의 전체 높이
};

// D2Render 클래스 정의
class D2Render {
public:
    D2Render();                      // 생성자: 기본값으로 객체 초기화
    ~D2Render();                     // 소멸자: 리소스 해제

    // 초기화 및 리소스 관리
    bool Initialize(HWND hwnd);      // D2D/DWrite 초기화 및 창 핸들과 연결
	void ExtendFrameIntoClient();  // 클라이언트 영역 확장
    void ResetFrameIntoClient(); // 클라이언트 영역 확장 해제
    void SetScreenSize(int width, int height);  // 렌더링 영역 크기 설정
    void Clear(COLORREF bgColor);    // 배경색으로 화면 지우기
    void BeginDraw();                // 그리기 작업 시작
    void EndDraw();                  // 그리기 작업 종료 및 화면 업데이트
    void Resize(int width, int height);  // 창 크기 변경 시 렌더 타겟 크기 조정
    void Shutdown();                 // 모든 D2D/DWrite 리소스 해제
    void PushClip(float left, float top, float right, float bottom);
    void PopClip();
    void ResetClips();

    // 이미지 로딩 및 렌더링 관련 메서드
    ID2D1Bitmap* LoadImageFromFile(const std::wstring& filePath);
    void DrawImage(ID2D1Bitmap* pBitmap, float x, float y);
    void DrawImage(ID2D1Bitmap* pBitmap, float x, float y, float width, float height);

    // 폰트 및 텍스트 설정
    void SetFont(std::wstring fontName, int fontSize, bool bold, bool italic);  // 폰트 설정
    void SetFontSize(int fontSize);  // 폰트 사이즈 설정
    void GetFont(std::wstring& fontName, int& fontSize, bool& bold, bool& italic);  // 현재 폰트 정보 가져오기
    int GetFontSize();  // 현재 폰트 사이즈 가져오기
    void SetSpacing(int spacing);  // 줄 간격 설정
    void SetTextColor(COLORREF textColor);     // 텍스트 색상 설정
    void SetOldTextColor(COLORREF textColor);  // 이전 텍스트 색상 설정
    void SetBgColor(COLORREF bgColor);         // 배경 색상 설정
    void SetSelectionColors(COLORREF textColor, COLORREF bgColor);  // 선택 영역 색상 설정

    // 배경 투명도 설정/조회 메서드 추가
    void SetBgOpacity(float opacity);
    float GetBgOpacity() const;
    // RenderTarget 접근자 추가
    ID2D1HwndRenderTarget* GetRenderTarget() const { return m_pRenderTarget; }

    // 텍스트 측정 및 분석
    float GetTextWidth(const std::wstring& line);  // 텍스트 문자열의 픽셀 너비 계산
    std::vector<int> MeasureTextPositions(const std::wstring& text);  // 텍스트 내의 각 문자 위치(오프셋)를 픽셀 단위로 측정
    TextMetrics GetTextMetrics() const;  // 현재 폰트의 메트릭스(높이, 간격 등) 정보 반환
    float GetLineHeight() const;     // 현재 폰트의 줄 높이 반환

    // 텍스트 그리기
    void FillSolidRect(const D2D1_RECT_F& rect, COLORREF color);  // 단색으로 사각형 채우기
    void DrawEditText(float x, float y, const D2D1_RECT_F* clipRect, const wchar_t* text, size_t length, bool isOld);  // 텍스트 그리기 (선택 없음)
    void DrawEditText(float x, float y, const D2D1_RECT_F* clipRect, const wchar_t* text,
        size_t length, bool selected, int startSelectPos, int endSelectPos, bool isOld);  // 텍스트 그리기 (부분 선택 가능)

    // 팩토리 접근자 추가
    ID2D1Factory* GetD2DFactory() const { return m_pD2DFactory; }
    IDWriteFactory* GetDWriteFactory() const { return m_pDWriteFactory; }
    IDWriteTextFormat* GetTextFormat() const { return m_pTextFormat; }

private:
    // DirectWrite 및 Direct2D 리소스
    CComPtr<ID2D1Factory> m_pD2DFactory;              // D2D 팩토리 인터페이스
    CComPtr<IDWriteFactory> m_pDWriteFactory;         // DirectWrite 팩토리 인터페이스
    CComPtr<ID2D1HwndRenderTarget> m_pRenderTarget;   // 창에 그리기 위한 렌더 타겟
    CComPtr<IDWriteTextFormat> m_pTextFormat;         // 일반 텍스트 포맷
    CComPtr<ID2D1SolidColorBrush> m_pTextBrush;       // 일반 텍스트 브러시
	CComPtr<ID2D1SolidColorBrush> m_pOldTextBrush;    // 이전 텍스트 브러시
    CComPtr<ID2D1SolidColorBrush> m_pBgBrush;         // 배경 브러시
    CComPtr<ID2D1SolidColorBrush> m_pSelectedTextBrush;  // 선택된 텍스트 브러시
    CComPtr<ID2D1SolidColorBrush> m_pSelectedBgBrush;    // 선택된 배경 브러시

    // 캐시된 텍스트 메트릭스
    TextMetrics m_textMetrics;       // 현재 폰트의 메트릭스 정보 저장

    // 폰트 설정
    std::wstring m_fontName;         // 폰트 이름 (예: "Consolas", "D2Coding")
    float m_fontSize;                // 폰트 크기 (포인트 단위)
    DWRITE_FONT_WEIGHT m_fontWeight; // 폰트 두께 (일반, 굵게 등)
    DWRITE_FONT_STYLE m_fontStyle;   // 폰트 스타일 (일반, 이탤릭 등)

    // 색상 설정
    D2D1::ColorF m_textColor;        // 텍스트 색상
    D2D1::ColorF m_oldTextColor;        // 이전 텍스트 색상
    D2D1::ColorF m_bgColor;          // 배경 색상
    D2D1::ColorF m_selectedTextColor;  // 선택된 텍스트 색상
    D2D1::ColorF m_selectedBgColor;    // 선택된 배경 색상

    // 배경 투명도 변수 추가 (0.0f ~ 1.0f)
    float m_bgOpacity;

    // 창 크기
    int m_width;                     // 렌더링 영역 너비
    int m_height;                    // 렌더링 영역 높이
    int m_spacing;                   // 줄 간격 (기본값: 0)
	//int m_skipLeft;                 // 왼쪽 여백 (기본값: 0)

    // WIC 관련 변수
    CComPtr<IWICImagingFactory> m_pWICFactory;

    // 초기화 상태
    bool m_initialized;              // D2D/DWrite 초기화 완료 여부
    HWND m_hwnd;

    // 내부 메소드
    void UpdateTextMetrics();        // 폰트 변경 시 텍스트 메트릭스 정보 업데이트
    bool CreateTextFormat();         // 텍스트 포맷 객체 생성
    bool CreateBrushes();            // 브러시 객체 생성
};