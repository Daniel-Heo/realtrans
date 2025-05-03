//﻿*******************************************************************************
//    파     일     명 : D2Render.cpp
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
#include "D2Render.h"
#include "Util.h"

// COLORREF를 D2D1::ColorF로 변환하는 헬퍼 함수
inline D2D1::ColorF ColorRefToColorF(COLORREF color, float opacity = 1.0f) {
    return D2D1::ColorF(
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        opacity  // 알파 채널 설정
    );
}

// D2Render 클래스 구현
D2Render::D2Render()
    : m_fontName(L"Consolas")
    , m_fontSize(16.0f)
    , m_fontWeight(DWRITE_FONT_WEIGHT_NORMAL)
    , m_fontStyle(DWRITE_FONT_STYLE_NORMAL)
    , m_textColor(D2D1::ColorF(D2D1::ColorF::White))
    , m_bgColor(D2D1::ColorF(D2D1::ColorF::Black))
    , m_selectedTextColor(D2D1::ColorF(D2D1::ColorF::White))
    , m_selectedBgColor(D2D1::ColorF(0.0f, 0.4f, 0.8f))
	, m_oldTextColor(D2D1::ColorF(D2D1::ColorF::Gray))
    , m_width(0) // 화면 가로 크기
    , m_height(0) // 화면 세로 크기
    , m_spacing(5)
    , m_initialized(false)
    , m_bgOpacity(1.0f)  // 배경 투명도 초기화
	, m_pD2DFactory(nullptr)
	, m_pDWriteFactory(nullptr)
	, m_pRenderTarget(nullptr)
	, m_pTextFormat(nullptr)
	, m_pTextBrush(nullptr)
	, m_pOldTextBrush(nullptr)
	, m_pBgBrush(nullptr)
	, m_pSelectedTextBrush(nullptr)
	, m_pSelectedBgBrush(nullptr)
	, m_pWICFactory(nullptr)
	, m_hwnd(nullptr)
{
    memset(&m_textMetrics, 0, sizeof(TextMetrics));
}

D2Render::~D2Render() {
    CoUninitialize(); // 컴포넌트 사용 해제
    Shutdown();
}

bool D2Render::Initialize(HWND hwnd) {
    if (m_initialized) {
        return true;
    }

    m_hwnd = hwnd;  // HWND 저장

    // COM 초기화가 되었는지 확인 (RealTrans.cpp에 이미 CoInitializeEx가 있는지 확인)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        // COM 초기화 실패 처리
        return false;
    }

    // Direct2D 팩토리 생성
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) {
        return false;
    }

    // DirectWrite 팩토리 생성
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
    );
    if (FAILED(hr)) {
        return false;
    }

    // 렌더 타겟 생성을 위한 속성 설정
    RECT rc;
    GetClientRect(hwnd, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    // 핵심 렌더링 품질 설정
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0.0f,  // DPI는 기본값
        0.0f,
        D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,  //D2D1_RENDER_TARGET_USAGE_NONE, D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE
        D2D1_FEATURE_LEVEL_DEFAULT
    );

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        hwnd,
        D2D1::SizeU(m_width, m_height),
        D2D1_PRESENT_OPTIONS_IMMEDIATELY
    );

    // 렌더 타겟 생성
    hr = m_pD2DFactory->CreateHwndRenderTarget(
        rtProps,
        hwndProps,
        &m_pRenderTarget
    );
    if (FAILED(hr)) {
        return false;
    }

    // 텍스트 렌더링 품질 설정
    m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    // 렌더 타겟 생성 후 블렌드 모드 설정
    if (m_pRenderTarget) {
        // Direct2D 1.1 이상에서는 ID2D1DeviceContext로 변환 후 설정 가능
        CComQIPtr<ID2D1DeviceContext> deviceContext(m_pRenderTarget);
        if (deviceContext) {
            // SOURCE_OVER 블렌드 모드 설정 - 알파 블렌딩에 적합
            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        }
    }

    // 텍스트 포맷 생성
    CreateTextFormat();

    // 브러시 생성
    CreateBrushes();

    // 텍스트 메트릭스 계산
    UpdateTextMetrics();

    // WIC 팩토리 생성
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pWICFactory)
    );
    if (FAILED(hr)) {
        return false;
    }

    m_initialized = true;
    return true;
}

void D2Render::ExtendFrameIntoClient() {
    if (!m_hwnd)
        return;

    MARGINS margins = { 0 };
    margins.cxLeftWidth = 0;
    margins.cxRightWidth = m_width;
    margins.cyTopHeight = 0;
    margins.cyBottomHeight = m_height;

    DwmExtendFrameIntoClientArea(m_hwnd, &margins);
}

void D2Render::ResetFrameIntoClient() {
    if (!m_hwnd)
        return;

    MARGINS margins = { 0, 0, 0, 0 }; // 모두 0으로 설정
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);
}

void D2Render::SetScreenSize(int width, int height) {
    if (m_width != width || m_height != height) {
        m_width = width;
        m_height = height;

        if (m_pRenderTarget) {
            m_pRenderTarget->Resize(D2D1::SizeU(width, height));
        }
    }
}

void D2Render::Clear(COLORREF bgColor) {
    if (!m_initialized || !m_pRenderTarget) {
        return;
    }

    // 배경을 먼저 완전 투명하게 클리어
    //D2D1::ColorF transparentColor = ColorRefToColorF(bgColor, 0.0f); // 투명 배경
    //m_pRenderTarget->Clear(transparentColor);
    m_pRenderTarget->Clear();

    // 지정된 색상과 투명도로 배경 그리기 (AlphaBlend 적용)
    CComPtr<ID2D1SolidColorBrush> bgBrush;
    D2D1::ColorF bgColorWithAlpha(
        GetRValue(bgColor) / 255.0f,
        GetGValue(bgColor) / 255.0f,
        GetBValue(bgColor) / 255.0f,
        m_bgOpacity  // 사용자가 설정한 투명도 적용
    );

    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(bgColorWithAlpha, &bgBrush);
    if (SUCCEEDED(hr) && bgBrush) {
        // 전체 창 영역을 반투명 배경색으로 채움
        D2D1_SIZE_F size = m_pRenderTarget->GetSize();
        D2D1_RECT_F rect = D2D1::RectF(0, 0, size.width, size.height);
        m_pRenderTarget->FillRectangle(rect, bgBrush);
    }
}

// BeginDraw 함수에 합성 모드 설정 추가
void D2Render::BeginDraw() {
    if (!m_initialized || !m_pRenderTarget) {
        return;
    }

    // 렌더 타겟의 상태 확인
    HRESULT windowState = m_pRenderTarget->CheckWindowState();
    if (windowState == D2D1_WINDOW_STATE_OCCLUDED) {
        TRACE(L"경고: 렌더 타겟이 가려져 있습니다\n");
    }

    m_pRenderTarget->BeginDraw();

    // Direct2D 1.1 이상에서 블렌드 모드 다시 설정
    CComQIPtr<ID2D1DeviceContext> deviceContext(m_pRenderTarget);
    if (deviceContext) {
        deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
    }
}

void D2Render::EndDraw() {
    if (!m_initialized || !m_pRenderTarget) {
        return;
    }

    HRESULT hr = m_pRenderTarget->EndDraw();
    if (FAILED(hr))
    {
        TRACE(L"Direct2D EndDraw 실패! HRESULT: %x\n", hr);
    }
}

void D2Render::Resize(int width, int height) {
    if (!m_initialized || !m_pRenderTarget) {
        return;
    }

    m_width = width;
    m_height = height;
    m_pRenderTarget->Resize(D2D1::SizeU(width, height));
}

void D2Render::Shutdown() {
    // 모든 COM 인터페이스 해제
    if (m_pSelectedBgBrush) m_pSelectedBgBrush.Release();
    if (m_pSelectedTextBrush) m_pSelectedTextBrush.Release();
    if (m_pBgBrush) m_pBgBrush.Release();
    if (m_pTextBrush) m_pTextBrush.Release();
    if(m_pOldTextBrush) m_pOldTextBrush.Release();
    if (m_pTextFormat) m_pTextFormat.Release();
    if (m_pRenderTarget) m_pRenderTarget.Release();
    if (m_pDWriteFactory) m_pDWriteFactory.Release();
    if (m_pD2DFactory) m_pD2DFactory.Release();
	if (m_pWICFactory) m_pWICFactory.Release();

    m_initialized = false;
}

// 그릴 영역 지정
void D2Render::PushClip(float left, float top, float right, float bottom) {
    if (m_pRenderTarget) {
        D2D1_RECT_F clipRect = D2D1::RectF(left, top, right, bottom);
        m_pRenderTarget->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
    }
}


// 그릴 영역 해제
void D2Render::PopClip() {
    if (m_pRenderTarget ) {
        m_pRenderTarget->PopAxisAlignedClip();
    }
}

ID2D1Bitmap* D2Render::LoadImageFromFile(const std::wstring& filePath) {
    if (!m_initialized || !m_pRenderTarget || !m_pWICFactory) {
        return nullptr;
    }

    // WIC 디코더 생성
    CComPtr<IWICBitmapDecoder> pDecoder = nullptr;
    HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
    );
    if (FAILED(hr)) {
        TRACE(L"이미지 디코더 생성 실패: %s\n", filePath.c_str());
        return nullptr;
    }

    // 프레임 가져오기 (대부분의 이미지는 단일 프레임)
    CComPtr<IWICBitmapFrameDecode> pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) {
        TRACE(L"이미지 프레임 가져오기 실패\n");
        return nullptr;
    }

    // 포맷 변환기 생성
    CComPtr<IWICFormatConverter> pConverter = nullptr;
    hr = m_pWICFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) {
        TRACE(L"포맷 변환기 생성 실패\n");
        return nullptr;
    }

    // 32bpp BGRA 형식으로 변환 (Direct2D에서 사용)
    hr = pConverter->Initialize(
        pFrame,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        NULL,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) {
        TRACE(L"이미지 포맷 변환 실패\n");
        return nullptr;
    }

    // Direct2D 비트맵 생성
    ID2D1Bitmap* pBitmap = nullptr;
    hr = m_pRenderTarget->CreateBitmapFromWicBitmap(
        pConverter,
        NULL,
        &pBitmap
    );
    if (FAILED(hr)) {
        TRACE(L"Direct2D 비트맵 생성 실패\n");
        return nullptr;
    }

    return pBitmap;
}

void D2Render::DrawImage(ID2D1Bitmap* pBitmap, float x, float y) {
    if (!m_initialized || !m_pRenderTarget || !pBitmap) {
        return;
    }

    // 이미지 크기 가져오기
    D2D1_SIZE_F size = pBitmap->GetSize();

    // 이미지 그리기
    m_pRenderTarget->DrawBitmap(
        pBitmap,
        D2D1::RectF(x, y, x + size.width, y + size.height)
    );
}

void D2Render::DrawImage(ID2D1Bitmap* pBitmap, float x, float y, float width, float height) {
    if (!m_initialized || !m_pRenderTarget || !pBitmap) {
        return;
    }

    // 이미지 그리기
    m_pRenderTarget->DrawBitmap(
        pBitmap,
        D2D1::RectF(x, y, x + width, y + height)
    );
}

void D2Render::SetFont(std::wstring fontName, int fontSize, bool bold, bool italic) {
    m_fontName = fontName;
    m_fontSize = static_cast<float>(fontSize);
    m_fontWeight = bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    m_fontStyle = italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

    if (m_initialized) {
        CreateTextFormat();
        UpdateTextMetrics();
    }
}

void D2Render::SetFontSize(int fontSize) {
    m_fontSize = static_cast<float>(fontSize);
    if (m_initialized) {
        CreateTextFormat();
        UpdateTextMetrics();
    }
}

void D2Render::GetFont(std::wstring& fontName, int& fontSize, bool& bold, bool& italic) {
    // 현재 폰트 정보 가져오기
    fontName = m_fontName;
    fontSize = static_cast<int>(m_fontSize);
    bold = (m_fontWeight == DWRITE_FONT_WEIGHT_BOLD);
    italic = (m_fontStyle == DWRITE_FONT_STYLE_ITALIC);
}

int D2Render::GetFontSize() {
    return m_fontSize;
}

void D2Render::SetSpacing(int spacing) {
    m_spacing = spacing;
}

void D2Render::SetTextColor(COLORREF textColor) {
    if (!m_initialized || !m_pRenderTarget) {
        m_textColor = ColorRefToColorF(textColor);
        return;
    }

    m_textColor = ColorRefToColorF(textColor);
    if (m_pTextBrush) {
        m_pTextBrush->SetColor(m_textColor);
    }
}

void D2Render::SetOldTextColor(COLORREF textColor) {
    if (!m_initialized || !m_pRenderTarget) {
        m_oldTextColor = ColorRefToColorF(textColor);
        return;
    }

    m_oldTextColor = ColorRefToColorF(textColor);
    if (m_pOldTextBrush) {
        m_pOldTextBrush->SetColor(m_oldTextColor);
    }
}

void D2Render::SetBgColor(COLORREF bgColor) {
    if (!m_initialized || !m_pRenderTarget) {
        m_bgColor = ColorRefToColorF(bgColor, GetBgOpacity());
        m_bgColor.a = m_bgOpacity;  // 투명도 설정
        return;
    }

    // 색상 업데이트 시 이전 투명도 유지
    m_bgColor = ColorRefToColorF(bgColor, GetBgOpacity());
    m_bgColor.a = m_bgOpacity;

    if (m_pBgBrush) {
        m_pBgBrush->SetColor(m_bgColor);
    }
}

void D2Render::SetSelectionColors(COLORREF textColor, COLORREF bgColor) {
    m_selectedTextColor = ColorRefToColorF(textColor);
    m_selectedBgColor = ColorRefToColorF(bgColor);

    if (m_pSelectedTextBrush) {
        m_pSelectedTextBrush->SetColor(m_selectedTextColor);
    }

    if (m_pSelectedBgBrush) {
        m_pSelectedBgBrush->SetColor(m_selectedBgColor);
    }
}

// 2.3. 배경 투명도 설정/조회 메서드 구현
void D2Render::SetBgOpacity(float opacity) {
    // 0.0f ~ 1.0f 범위로 제한
    m_bgOpacity = max(0.0f, min(1.0f, opacity));

    // 배경 브러시가 존재하면 투명도 업데이트
    if (m_pBgBrush) {
        D2D1::ColorF color = m_bgColor;
        color.a = m_bgOpacity;
        m_pBgBrush->SetColor(color);
    }
}

float D2Render::GetBgOpacity() const {
    return m_bgOpacity;
}

float D2Render::GetTextWidth(const std::wstring& line) {
    if (!m_initialized || !m_pDWriteFactory || !m_pTextFormat || line.empty()) {
        return 0.0f;
    }

    CComPtr<IDWriteTextLayout> textLayout;
    HRESULT hr = m_pDWriteFactory->CreateTextLayout(
        line.c_str(),
        static_cast<UINT32>(line.length()),
        m_pTextFormat,
        static_cast<float>(m_width * 2),  // 넉넉한 최대 너비
        static_cast<float>(m_textMetrics.lineHeight),
        &textLayout
    );

    if (FAILED(hr) || !textLayout) {
        return 0.0f;
    }

    DWRITE_TEXT_METRICS metrics;
    hr = textLayout->GetMetrics(&metrics);
    if (FAILED(hr)) {
        return 0;
    }
    return metrics.widthIncludingTrailingWhitespace;
}

// 텍스트 내의 각 문자 위치(오프셋)를 픽셀 단위로 측정
std::vector<int> D2Render::MeasureTextPositions(const std::wstring& text) {
    std::vector<int> positions;
    if (!m_initialized || !m_pDWriteFactory || !m_pTextFormat || text.empty()) {
        return positions;
    }

    CComPtr<IDWriteTextLayout> textLayout;
    HRESULT hr = m_pDWriteFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_pTextFormat,
        static_cast<float>(m_width * 2),  // 넉넉한 최대 너비
        static_cast<float>(m_textMetrics.lineHeight),
        &textLayout
    );

    if (FAILED(hr) || !textLayout) {
        return positions;
    }

    positions.resize(text.length() + 1, 0);

    for (size_t i = 0; i <= text.length(); ++i) {
        DWRITE_HIT_TEST_METRICS hitTestMetrics;
        float pointX, pointY;
        hr = textLayout->HitTestTextPosition(
            static_cast<UINT32>(i),
            FALSE,
            &pointX,
            &pointY,
            &hitTestMetrics
        );

        if (SUCCEEDED(hr)) {
            positions[i] = static_cast<int>(pointX);
        }
    }

    return positions;
}

TextMetrics D2Render::GetTextMetrics() const {
    return m_textMetrics;
}

float D2Render::GetLineHeight() const {
    return m_textMetrics.lineHeight;
}

void D2Render::FillSolidRect(const D2D1_RECT_F& rect, COLORREF color) {
    if (!m_initialized || !m_pRenderTarget) {
        return;
    }

    // 배경색인지 확인 (D2D1::ColorF와 COLORREF 비교)
    // COLORREF를 R, G, B 값으로 분해
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);

    // m_bgColor(D2D1::ColorF)의 R, G, B 값과 비교 (0-255 범위로 변환)
    int bgR = static_cast<int>(m_bgColor.r * 255.0f);
    int bgG = static_cast<int>(m_bgColor.g * 255.0f);
    int bgB = static_cast<int>(m_bgColor.b * 255.0f);

    bool isBackgroundColor = (r == bgR && g == bgG && b == bgB);

    // 배경색인 경우 투명도 적용, 아니면 불투명
    float opacity = isBackgroundColor ? m_bgOpacity : 1.0f;

    // 브러시 생성 및 사각형 채우기
    CComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(
        ColorRefToColorF(color, opacity),
        &brush
    );

    if (SUCCEEDED(hr)) {
        m_pRenderTarget->FillRectangle(rect, brush);
    }
}

void D2Render::DrawEditText(float x, float y, const D2D1_RECT_F* clipRect, const wchar_t* text, size_t length, bool isOld) {
    DrawEditText(x, y, clipRect, text, length, false, 0, length - 1, isOld);
}

void D2Render::DrawEditText(float x, float y, const D2D1_RECT_F* clipRect, const wchar_t* text,
    size_t length, bool selected, int startSelectPos, int endSelectPos, bool isOld) {
    // 유효성 검사 추가
    if (!m_initialized || !m_pRenderTarget || !text || length == 0) {
        return;
    }

    // 텍스트 포맷 체크
    if (!m_pTextFormat) {
        if (!CreateTextFormat()) {
            return; // 텍스트 포맷 생성 실패
        }
    }

    // 브러시 체크 및 생성
    if (!isOld && !m_pTextBrush) {
        if (!CreateBrushes()) {
            return; // 브러시 생성 실패
        }
	}
	else if (isOld && !m_pOldTextBrush) {
		if (!CreateBrushes()) {
			return; // 브러시 생성 실패
		}
	}

    // 선택 범위 조정
    if (selected && startSelectPos >= endSelectPos) {
        // 전체 텍스트 선택 (기존 동작)
        startSelectPos = 0;
        endSelectPos = static_cast<int>(length);
    }

    // 부분 선택 여부 확인
    bool isPartialSelection = selected && startSelectPos != endSelectPos && (startSelectPos > 0 || endSelectPos < static_cast<int>(length));

    // 전체 선택이면 기존 동작 사용
    if (selected && !isPartialSelection) {
        // 선택 배경 그리기
        ID2D1Brush* textBrush = m_pSelectedTextBrush ? m_pSelectedTextBrush : isOld ? m_pOldTextBrush : m_pTextBrush;

        D2D1_RECT_F textRect = D2D1::RectF(
            x,
            y,
            x + GetTextWidth(std::wstring(text, length)),
            y + m_textMetrics.lineHeight + m_spacing
        );

        if (clipRect) {
            // 클리핑 영역과 교차
            textRect.left = max(textRect.left, clipRect->left);
            textRect.top = max(textRect.top, clipRect->top);
            textRect.right = min(textRect.right, clipRect->right);
            textRect.bottom = min(textRect.bottom, clipRect->bottom);
        }

        if (textRect.right > textRect.left && textRect.bottom > textRect.top) {
            m_pRenderTarget->FillRectangle(textRect, m_pSelectedBgBrush);
        }

        // 클리핑 설정
        bool clippingPushed = false;
        if (clipRect) {
            m_pRenderTarget->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            clippingPushed = true;
        }

        // 텍스트 그리기
        D2D1_RECT_F layoutRect = D2D1::RectF(
            x,
            y,
            x + static_cast<float>(m_width),
            y + m_textMetrics.lineHeight
        );

        try {
            m_pRenderTarget->DrawText(
                text,
                static_cast<UINT32>(length),
                m_pTextFormat,
                layoutRect,
                textBrush,
                D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );
        }
        catch (...) {
            // 예외 처리
        }

        // 클리핑 해제
        if (clippingPushed) {
            m_pRenderTarget->PopAxisAlignedClip();
        }
    }
    // 부분 선택
    else if (isPartialSelection) {
        // 부분 선택 처리

        // 1. 선택 영역 전의 텍스트 그리기
        if (startSelectPos > 0) {
            D2D1_RECT_F layoutRect = D2D1::RectF(
                x,
                y,
                x + static_cast<float>(m_width),
                y + m_textMetrics.lineHeight
            );

            bool clippingPushed = false;
            if (clipRect) {
                m_pRenderTarget->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
                clippingPushed = true;
            }

            try {
                m_pRenderTarget->DrawText(
                    text,
                    static_cast<UINT32>(startSelectPos),
                    m_pTextFormat,
                    layoutRect,
                    isOld ? m_pOldTextBrush : m_pTextBrush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
                );
            }
            catch (...) {
                // 예외 처리
            }

            if (clippingPushed) {
                m_pRenderTarget->PopAxisAlignedClip();
            }
        }

        // 2. 선택된 텍스트 부분 너비 계산
        std::wstring prePart(text, startSelectPos);
        float preWidth = GetTextWidth(prePart);

        std::wstring selectedPart(&text[startSelectPos], endSelectPos - startSelectPos);
        float selectWidth = GetTextWidth(selectedPart);

        // 3. 선택된 부분의 배경 그리기
        D2D1_RECT_F selectRect = D2D1::RectF(
            x + preWidth,
            y,
            x + preWidth + selectWidth,
            y + m_textMetrics.lineHeight + m_spacing
        );

        if (clipRect) {
            // 클리핑 영역과 교차
            selectRect.left = max(selectRect.left, clipRect->left);
            selectRect.top = max(selectRect.top, clipRect->top);
            selectRect.right = min(selectRect.right, clipRect->right);
            selectRect.bottom = min(selectRect.bottom, clipRect->bottom);
        }

        if (selectRect.right > selectRect.left && selectRect.bottom > selectRect.top) {
            m_pRenderTarget->FillRectangle(selectRect, m_pSelectedBgBrush);
        }

        // 4. 선택된 텍스트 그리기
        D2D1_RECT_F layoutRect = D2D1::RectF(
            x + preWidth,
            y,
            x + preWidth + selectWidth,
            y + m_textMetrics.lineHeight
        );

        bool clippingPushed = false;
        if (clipRect) {
            m_pRenderTarget->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            clippingPushed = true;
        }

        try {
            m_pRenderTarget->DrawText(
                &text[startSelectPos],
                static_cast<UINT32>(endSelectPos - startSelectPos),
                m_pTextFormat,
                layoutRect,
                m_pSelectedTextBrush ? m_pSelectedTextBrush : isOld ? m_pOldTextBrush : m_pTextBrush,
                D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );
        }
        catch (...) {
            // 예외 처리
        }

        if (clippingPushed) {
            m_pRenderTarget->PopAxisAlignedClip();
        }

        // 5. 선택 영역 이후의 텍스트 그리기
        if (endSelectPos < static_cast<int>(length)) {
            float postX = x + preWidth + selectWidth;

            D2D1_RECT_F layoutRect = D2D1::RectF(
                postX,
                y,
                postX + static_cast<float>(m_width),
                y + m_textMetrics.lineHeight
            );

            clippingPushed = false;
            if (clipRect) {
                m_pRenderTarget->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
                clippingPushed = true;
            }

            try {
                m_pRenderTarget->DrawText(
                    &text[endSelectPos],
                    static_cast<UINT32>(length - endSelectPos),
                    m_pTextFormat,
                    layoutRect,
                    isOld ? m_pOldTextBrush : m_pTextBrush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
                );
            }
            catch (...) {
                // 예외 처리
            }

            if (clippingPushed) {
                m_pRenderTarget->PopAxisAlignedClip();
            }
        }
    }
    // 선택되지 않은 일반 텍스트 그리기
    else {
        // 텍스트 그리기 (텍스트는 불투명하게 유지)
        D2D1_RECT_F layoutRect = D2D1::RectF(
            x,
            y,
            x + static_cast<float>(m_width),
            y + m_textMetrics.lineHeight
        );

        bool clippingPushed = false;
        if (clipRect) {
            m_pRenderTarget->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            clippingPushed = true;
        }

        try {
            m_pRenderTarget->DrawText(
                text,
                static_cast<UINT32>(length),
                m_pTextFormat,
                layoutRect,
                isOld ? m_pOldTextBrush : m_pTextBrush,  // 텍스트는 불투명 브러시 사용
                D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );
        }
        catch (...) {
            // 예외 처리
        }

        if (clippingPushed) {
            m_pRenderTarget->PopAxisAlignedClip();
        }
    }
}

void D2Render::UpdateTextMetrics() {
    if (!m_initialized || !m_pDWriteFactory || !m_pTextFormat) {
        return;
    }

    // 폰트 정보 조회를 위한 텍스트 레이아웃 생성
    CComPtr<IDWriteTextLayout> textLayout;
    HRESULT hr = m_pDWriteFactory->CreateTextLayout(
        L"Aygj|한글",  // 여러 문자를 포함하여 측정
        5,
        m_pTextFormat,
        static_cast<float>(m_width),
        static_cast<float>(m_height),
        &textLayout
    );

    DWRITE_TEXT_METRICS textMetrics;
    hr = textLayout->GetMetrics(&textMetrics);
    if (FAILED(hr)) {
        return;
    }

    DWRITE_LINE_METRICS lineMetrics;
    UINT32 lineCount = 1;
    hr = textLayout->GetLineMetrics(&lineMetrics, 1, &lineCount);
    if (FAILED(hr) || lineCount == 0) {
        return;
    }

    // 폰트 콜렉션 및 폰트 패밀리 가져오기
    CComPtr<IDWriteFontCollection> fontCollection;
    hr = m_pTextFormat->GetFontCollection(&fontCollection);
    if (FAILED(hr) || !fontCollection) {
        return;
    }

    UINT32 fontFamilyIndex;
    BOOL fontFamilyExists;
    hr = fontCollection->FindFamilyName(m_fontName.c_str(), &fontFamilyIndex, &fontFamilyExists);
    if (FAILED(hr) || !fontFamilyExists) {
        return;
    }

    CComPtr<IDWriteFontFamily> fontFamily;
    hr = fontCollection->GetFontFamily(fontFamilyIndex, &fontFamily);
    if (FAILED(hr) || !fontFamily) {
        return;
    }

    // 폰트 정보 가져오기
    CComPtr<IDWriteFont> font;
    hr = fontFamily->GetFirstMatchingFont(m_fontWeight, DWRITE_FONT_STRETCH_NORMAL, m_fontStyle, &font);
    if (FAILED(hr) || !font) {
        return;
    }

    // 메트릭스 가져오기
    DWRITE_FONT_METRICS fontMetrics;
    font->GetMetrics(&fontMetrics);

    // 텍스트 메트릭스 업데이트
    float designUnitsToPixels = m_fontSize / fontMetrics.designUnitsPerEm;

    m_textMetrics.ascent = fontMetrics.ascent * designUnitsToPixels;
    m_textMetrics.descent = fontMetrics.descent * designUnitsToPixels;
    m_textMetrics.lineGap = fontMetrics.lineGap * designUnitsToPixels;
    m_textMetrics.capHeight = fontMetrics.capHeight * designUnitsToPixels;
    m_textMetrics.xHeight = fontMetrics.xHeight * designUnitsToPixels;
    m_textMetrics.underlinePosition = fontMetrics.underlinePosition * designUnitsToPixels;
    m_textMetrics.underlineThickness = fontMetrics.underlineThickness * designUnitsToPixels;
    m_textMetrics.strikethroughPosition = fontMetrics.strikethroughPosition * designUnitsToPixels;
    m_textMetrics.strikethroughThickness = fontMetrics.strikethroughThickness * designUnitsToPixels;
    m_textMetrics.lineHeight = lineMetrics.height;
}

bool D2Render::CreateTextFormat() {
    if (!m_pDWriteFactory) {
        return false;
    }

    // 기존 텍스트 포맷 해제
    m_pTextFormat = nullptr;

    // 메인 텍스트 포맷 생성
    HRESULT hr = m_pDWriteFactory->CreateTextFormat(
        m_fontName.c_str(),
        nullptr,
        m_fontWeight,
        m_fontStyle,
        DWRITE_FONT_STRETCH_NORMAL,
        m_fontSize,
        L"ko-kr",
        &m_pTextFormat
    );

    if (FAILED(hr) || !m_pTextFormat) {
        // 실패 시 기본 폰트 사용
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Arial", // 기본 고정폭 폰트
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"en-us",
            &m_pTextFormat
        );

        if (FAILED(hr) || !m_pTextFormat) {
            return false;
        }
    }

    // 텍스트 정렬 설정
    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    m_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    return true;
}

bool D2Render::CreateBrushes() {
    if (!m_pRenderTarget) {
        return false;
    }

    // 텍스트 브러시 생성 - 항상 불투명
    if (!m_pTextBrush) {
        HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(m_textColor, &m_pTextBrush);
        if (FAILED(hr) || !m_pTextBrush) {
            return false;
        }
    }
    else {
        m_pTextBrush->SetColor(m_textColor);
    }

    // 배경 브러시 생성 - 투명도 적용
    D2D1::ColorF bgColorWithOpacity = m_bgColor;
    bgColorWithOpacity.a = m_bgOpacity;  // 투명도 설정

    if (!m_pBgBrush) {
        HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(bgColorWithOpacity, &m_pBgBrush);
        if (FAILED(hr)) {
            // 배경 브러시 실패는 치명적이지 않음
        }
    }
    else {
        m_pBgBrush->SetColor(bgColorWithOpacity);
    }

    // 선택된 텍스트 브러시 생성 - 항상 불투명
    if (!m_pSelectedTextBrush) {
        HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(m_selectedTextColor, &m_pSelectedTextBrush);
        if (FAILED(hr)) {
            // 선택된 텍스트 브러시 실패는 치명적이지 않음
        }
    }
    else {
        m_pSelectedTextBrush->SetColor(m_selectedTextColor);
    }

    // 선택된 배경 브러시 생성 - 선택 영역은 불투명하게 유지
    if (!m_pSelectedBgBrush) {
        HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(m_selectedBgColor, &m_pSelectedBgBrush);
        if (FAILED(hr)) {
            // 선택된 배경 브러시 실패는 치명적이지 않음
        }
    }
    else {
        m_pSelectedBgBrush->SetColor(m_selectedBgColor);
    }

    // OldText 브러시 생성
    if (!m_pOldTextBrush) {
        HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(m_oldTextColor, &m_pOldTextBrush);
        if (FAILED(hr)) {
            // 이전 텍스트 브러시 실패는 치명적이지 않음
        }
    }
    else {
        m_pOldTextBrush->SetColor(m_oldTextColor);
    }

    return true;
}

//void D2Render::LogRenderTargetState() {
//    TRACE(L"--- D2Render 상태 ---\n");
//    TRACE(L"초기화 상태: %s\n", m_initialized ? L"초기화됨" : L"초기화되지 않음");
//    TRACE(L"렌더타겟: %p\n", m_pRenderTarget.p);
//
//    if (m_pRenderTarget) {
//        D2D1_SIZE_F size = m_pRenderTarget->GetSize();
//        TRACE(L"렌더타겟 크기: 너비=%.1f, 높이=%.1f\n", size.width, size.height);
//
//        HRESULT testState = m_pRenderTarget->CheckWindowState();
//        TRACE(L"윈도우 상태: %s (0x%08X)\n",
//            (testState == D2D1_WINDOW_STATE_OCCLUDED) ? L"가려짐" :
//            (testState == S_OK) ? L"정상" : L"기타",
//            testState);
//    }
//    TRACE(L"----------------------\n");
//}