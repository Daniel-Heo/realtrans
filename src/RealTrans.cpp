#include "RealTrans.h"
#include "NemoViewer.h"
#include "CDlgSummary.h"
#include "CDlgInfo.h"
#include "CDlgTrans.h"
#include "BgJob.h"
#include "CSettings.h"
#include "StrConvert.h"
#include "version.h"
#include "language.h"
#include "Util.h"

using json = nlohmann::json;

// 싱글톤 인스턴스 초기화
RealTrans* RealTrans::s_instance = nullptr;

// 환경정보
json settings;

// Static 콜백 함수 구현
static void AddTextToViewerCallback(const WCHAR* text) {
    RealTrans* pInstance = RealTrans::GetInstance();
    pInstance->AddTextToViewer(text);
}

RealTrans::RealTrans()
    : m_hWnd(NULL)
    , m_hInst(NULL)
    , m_pDlgSum(nullptr)
    , m_pDlgInfo(nullptr)
    , m_pDlgTrans(nullptr)
    , m_pBgJob(nullptr)
    , m_isTransWinActive(false)
    , m_isMouseInside(true)
    , m_bStopMenu(FALSE)
    , m_bMenuHide(false)
    , m_bSetupActive(false)
    , m_oldUILang(nullptr)
{
    // 아이콘 영역 초기 설정
    m_iconRect = { 500, 3, 34, 34 };
}

RealTrans::~RealTrans() {
    // BgJob 종료 요청
    if (m_pBgJob) {
        m_pBgJob->RequestExit();
    }

    // 다이얼로그 객체 해제
    if (m_pDlgSum) {
        delete m_pDlgSum;
        m_pDlgSum = nullptr;
    }

    if (m_pDlgInfo) {
        delete m_pDlgInfo;
        m_pDlgInfo = nullptr;
    }

    if (m_pDlgTrans) {
        delete m_pDlgTrans;
        m_pDlgTrans = nullptr;
    }
}

// 싱글톤 접근자
RealTrans* RealTrans::GetInstance() {
    if (s_instance == nullptr) {
        s_instance = new RealTrans();
    }
    return s_instance;
}

// 텍스트 포맷 설정 (새 텍스트용)
void RealTrans::SetNemoViewerConfig() {
    if (m_pViewer) {
        // 폰트 설정
        m_pViewer->SetFont(L"Arial", static_cast<int>(settings["cb_font_size"]), true, false);
        // 마지막줄 폰트 크기 및 색상 설정
        m_pViewer->SetTextColor(HexToCOLORREF(settings["ed_font_color"]));
        // 이전줄 폰트 크기 및 색상 설정
        m_pViewer->SetOldTextColor(HexToCOLORREF(settings["ed_oldfont_color"]));

        // 배경색 설정
        m_pViewer->SetBgColor(HexToCOLORREF(settings["bg_color"]));
        // 배경 투명도 설정 (settings["transparent"]는 0-100 범위)
        float bgOpacity = settings["transparent"] / 100.0f;
        m_pViewer->SetBgOpacity(bgOpacity);
        m_pViewer->SetMargin(7, 7, 7, 7);
    }
}

// 제일 마지막 줄이 보이도록
void RealTrans::ScrollToBottom() {
    if (m_pViewer) {
        // 텍스트의 마지막 부분으로 스크롤
        size_t lineCount = m_pViewer->GetLineCount();
        if (lineCount > 0) {
            m_pViewer->GotoLine(lineCount - 1);
        }
    }
}

void RealTrans::ScrollToBottomSel() {
    if (m_pViewer) {
        // 캐럿을 텍스트의 끝으로 이동
        size_t lineCount = m_pViewer->GetLineCount();
        if (lineCount > 0) {
            m_pViewer->GotoLine(lineCount - 1);
            m_pViewer->ClearSelection();
        }
    }
}

// NemoViewer에 텍스트 추가
void RealTrans::AddTextToViewer(const WCHAR* textToAdd) {
    if (m_pViewer) {
        // 텍스트 추가
        m_pViewer->AddText(textToAdd);

        // 마지막 위치로 스크롤
        ScrollToBottom();
    }
}

// 윈도우 크기 및 위치 조정
void RealTrans::ResizeWindow(int width, int height) {
    if (m_hWnd) {
        // 현재 윈도우 위치 가져오기
        RECT rcWindow;
        GetWindowRect(m_hWnd, &rcWindow);

        // 윈도우 크기 변경
        SetWindowPos(m_hWnd, NULL,
            rcWindow.left, rcWindow.top,
            width, height,
            SWP_NOZORDER | SWP_NOACTIVATE);

        // 뷰어 크기 조정
        if (m_pViewer && !m_isTransWinActive) {
            m_pViewer->Resize(width, height);

            // 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
            if (m_isMouseInside) {
                m_pViewer->SetBgOpacity(1);
                m_pViewer->SetMenuAreaWidth(LEFT_MENU_WIDTH);
            }
            else {
                float bgOpacity = settings["transparent"] / 100.0f;
                m_pViewer->SetBgOpacity(bgOpacity);
                m_pViewer->SetMenuAreaWidth(0);
            }
        }
    }
}

// 메뉴 표시/숨김 제어
void RealTrans::ShowMenu(bool show) {
    m_bMenuHide = !show;

    if (m_hWnd) {
        if (show) {
            // 메뉴 표시
            if (m_pViewer) {
                m_pViewer->SetMenuAreaWidth(LEFT_MENU_WIDTH);
            }

            // 타이틀바 및 테두리를 표시
            SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_EX_LAYERED);
            SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        }
        else {
            // 메뉴 숨김
            if (m_pViewer) {
                m_pViewer->SetMenuAreaWidth(0);
            }

            // 타이틀바 및 테두리를 숨김
            SetWindowLong(m_hWnd, GWL_EXSTYLE, WS_EX_LAYERED); // 확장 스타일을 모두 제거
            SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        }

        // 변경사항 적용
        SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

        // 내용 다시 그리기
        InvalidateRect(m_hWnd, NULL, TRUE);
    }
}

// 설정 불러오기
void RealTrans::LoadSettings(const std::string& filePath) {
    CSettings::GetInstance()->ReadSettings(filePath);
}

// 현재 실행 경로 설정
void RealTrans::SetExecutePath() {
    WCHAR strPath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, strPath, MAX_PATH);
    std::wstring wstrPath = strPath;
    char chPath[MAX_PATH];
    UTF16toUTF8(wstrPath, chPath);
    m_strExecutePath = chPath;
#ifndef NDEBUG // 디버그 모드
    for (int i = 0; i < 3; i++) {
        m_strExecutePath = m_strExecutePath.substr(0, m_strExecutePath.find_last_of("\\")); // 실행파일 끊고, 실행경로에서 2단계 위로 올라가야함
    }
#else
    for (int i = 0; i < 1; i++) {
        m_strExecutePath = m_strExecutePath.substr(0, m_strExecutePath.find_last_of("\\")); // 실행파일만 끊음.
    }
#endif
    m_strExecutePath = m_strExecutePath + "\\";
    SetCurrentDirectory(StringToWString(m_strExecutePath).c_str());
}

void RealTrans::SetSummaryFont() { 
    if (m_pDlgSum) m_pDlgSum->SetFont(); 
}

// 윈도우 클래스 등록
ATOM RealTrans::RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = RealTrans::StaticWndProc; // 정적 중개 함수 사용
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REALTRANS));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = APP_TITLE;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_REALTRANS));
    wcex.hbrBackground = CreateSolidBrush(RGB(5, 5, 5)); // 검은색 배경

    return RegisterClassExW(&wcex);
}

// 인스턴스 초기화
BOOL RealTrans::InitInstance(HINSTANCE hInstance, int nCmdShow) {
    LoadLibrary(TEXT("Msftedit.dll"));
    LoadLibrary(TEXT("ole32.dll"));
    LoadLibrary(TEXT("uuid.dll"));
    m_hInst = hInstance; // 인스턴스 핸들을 멤버 변수에 저장

    m_hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, // | WS_EX_LAYERED, // 확장 스타일
        APP_TITLE,      // 윈도우 클래스 이름
        APP_TITLE_VERSION, // 윈도우 타이틀 (이 경우에는 보이지 않음)
        WS_OVERLAPPEDWINDOW & ~WS_CAPTION, // 기본 스타일에서 WS_CAPTION을 제거 : WS_POPUP, WS_OVERLAPPEDWINDOW
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 400, // 위치 및 크기
        NULL, NULL, hInstance, NULL
    );

    if (!m_hWnd) return FALSE;

    // 요약창 생성
    m_pDlgSum = new CDlgSummary(hInstance);

    // BgJob에 다이얼로그 참조 전달
    if (m_pBgJob) {
        m_pBgJob->SetSummaryDialog(m_pDlgSum);
    }

    // NEMO Edit 초기화 및 BgJob에 등록
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (!FAILED(hr)) {
        m_pViewer = std::make_unique<NemoViewer>();
        if (m_pViewer->Initialize(m_hWnd)) {
            SetNemoViewerConfig();

            // BgJob에 뷰어 설정
            if (m_pBgJob) {
                m_pBgJob->SetViewer(m_pViewer.get());
            }
        }
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return TRUE;
}

// 메인 애플리케이션 실행
int RealTrans::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // 경로 설정
    SetExecutePath();

    // 환경설정값 불러오기
    LoadSettings("config.json");

    // UI 언어 설정
    GetUserLocale(&m_oldUILang);
    if (settings["ui_lang"] == 0) {
        WCHAR strTmp[][6] = { L"ko-KR", L"en-US", L"de-DE", L"es-ES", L"fr-FR", L"it-IT", L"ja-JP", L"pt-BR", L"ru-RU", L"zh-CN", L"ar-SA", L"hi-IN" };
        int size = 12;
        // 문자열이 strTmp에 존재하는지 확인
        bool exists = std::any_of(std::begin(strTmp), std::end(strTmp), [&](const WCHAR* s) { return lstrcmpW(s, m_oldUILang) == 0; });
        if (exists == false) SetUserLocale(L"en-US");
        else SetUserLocale(m_oldUILang);
    }
    else {
        switch (settings["ui_lang"].get<int>()) {
        case 1: SetUserLocale(L"ko-KR"); break; // 한국어 ( Korean )
        case 2: SetUserLocale(L"en-US"); break; // 영어 ( English )
        case 3: SetUserLocale(L"de-DE"); break; // 독일어 ( German )
        case 4: SetUserLocale(L"es-ES"); break; // 스페인어 ( Spanish ) 
        case 5: SetUserLocale(L"fr-FR"); break; // 프랑스어 ( French )
        case 6: SetUserLocale(L"it-IT"); break; // 이탈리아어 ( Italian )
        case 7: SetUserLocale(L"ja-JP"); break; // 일본어 ( Japanese )
        case 8: SetUserLocale(L"pt-BR"); break; // 포르투갈어 ( Portuguese )
        case 9: SetUserLocale(L"ru-RU"); break; // 러시아어 ( Russian )
        case 10: SetUserLocale(L"zh-CN"); break; // 중국어 ( Chinese ) 
        case 11: SetUserLocale(L"ar-SA"); break; // 아랍어 ( Arabic ) 
        case 12: SetUserLocale(L"hi-IN"); break; // 힌디어 ( Hindi ) 
        default: SetUserLocale(L"en-US");
        }
    }

    // BgJob 생성 및 초기화
    m_pBgJob = BgJob::GetInstance();
    m_pBgJob->Initialize();
    m_pBgJob->StartThreads();

    // 윈도우 클래스 등록
    RegisterWindowClass(hInstance);

    // 애플리케이션 초기화
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REALTRANS));

    // 타이머 실행
    m_pBgJob->RunTimer();

    MSG msg;

    // 기본 메시지 루프
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

// 디스패처
LRESULT CALLBACK RealTrans::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RealTrans* pThis = RealTrans::GetInstance();
    return pThis->WndProc(hWnd, message, wParam, lParam);
}

LRESULT RealTrans::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (m_pDlgTrans != NULL && m_pDlgTrans->bVisible == TRUE)
        m_pDlgTrans->WndProc(hWnd, message, wParam, lParam);

    switch (message) {
    case WM_CREATE:
    {
        // 윈도우 투명도 설정
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);

        // 타이틀 아이콘의 초기 위치 설정
        m_iconRect = { 500, 3, 34, 34 }; // 실제 프로젝트에서는 타이틀바의 위치와 크기를 계산하여 설정

        // 창을 항상 맨 위에 위치하게 설정
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 0.5초 간격으로 타이머 설정
        SetTimer(hWnd, TIMER_ID, 500, NULL);
    }
    break;
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS* pPos = (WINDOWPOS*)lParam;
        // 창이 최상위인지 확인
        LONG style = GetWindowLong(hWnd, GWL_EXSTYLE);
        if (!(style & WS_EX_TOPMOST)) {
            // 최상위가 아니면 최상위로 설정
            if (!(pPos->flags & SWP_NOZORDER)) {
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                return 0; // 메시지 처리 종료
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId) {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_ERASEBKGND:
        return 1; // 배경 지우기를 막음
    case WM_PAINT:
    {
        if (m_pDlgTrans == NULL || m_pDlgTrans->bVisible == FALSE) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            if (m_pViewer) {
                m_pViewer->Paint(hdc);
            }

            EndPaint(hWnd, &ps);
        }
    }
    break;
    case WM_LBUTTONDOWN:
    {
        POINTS pt = MAKEPOINTS(lParam);

        // 특정 영역 예: (100, 100) ~ (200, 200)
        if (pt.x > 0 && pt.x < 34 && pt.y > 0 && pt.y < 44) {
            // 환경설정
            m_bSetupActive = true;

            // 싱글톤 인스턴스를 통해 다이얼로그 생성
            CSettings::GetInstance()->Create(hWnd);

            m_bSetupActive = false;
            SetNemoViewerConfig();

            // 배경 투명도 설정
            if (m_isMouseInside && m_pViewer) {
                m_pViewer->SetBgOpacity(1);
            }
            else {
                float bgOpacity = settings["transparent"] / 100.0f;
                if (m_pViewer) m_pViewer->SetBgOpacity(bgOpacity);
            }
            //// 환경설정
            //m_bSetupActive = true;
            //DialogBox(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_SETUP), hWnd, CSettings::StaticDialogProc);
            //m_bSetupActive = false;

            //SetNemoViewerConfig();
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 44 && pt.y < 81) {
            // 요약
            m_pDlgSum->Create(hWnd);
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 81 && pt.y < 113) {
            // 실행/중지
            m_bStopMenu = !m_bStopMenu;

            // pViewer에 메뉴 상태 전달
            if (m_pViewer) {
                m_pViewer->SetMenuState(m_bStopMenu);
            }
            // json 파일로 저장
            if (m_bStopMenu == TRUE) {
                CSettings::GetInstance()->MakeChildCmd("xx", "xx");
            }
            else {
                std::string tgt_lang;
                if (settings["ck_pctrans"] == true) tgt_lang = settings["cb_pctrans_lang"];
                else tgt_lang = "xx";
                CSettings::GetInstance()->MakeChildCmd(settings["cb_voice_lang"], tgt_lang);
            }
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 113 && pt.y < 140) {
            ShellExecuteA(0, "open", "chrome.exe", "https://www.youtube.com/@markets/streams", NULL, SW_SHOWNORMAL);
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 140 && pt.y < 168) {
            ShellExecuteA(0, "open", "chrome.exe", "https://www.financialjuice.com/home", NULL, SW_SHOWNORMAL);
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 168 && pt.y < 197) {
            ShellExecuteA(0, "open", "chrome.exe", "https://edition.cnn.com/markets/fear-and-greed", NULL, SW_SHOWNORMAL);
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 197 && pt.y < 226) {
            // 번역창 전환
            if (m_pDlgTrans == NULL) {
                m_pDlgTrans = new CDlgTrans(m_hInst);
                m_pDlgTrans->Create(hWnd);

                // BgJob에 번역창 참조 전달
                if (m_pBgJob) {
                    m_pBgJob->SetTransDialog(m_pDlgTrans);
                }
            }

            m_isTransWinActive = m_pDlgTrans->bVisible ? false : true;

            if (m_pDlgTrans->bVisible == TRUE) {
                m_pDlgTrans->Hide();
                if (m_pViewer) {
                    RECT client;
                    GetClientRect(hWnd, &client);
                    m_pViewer->SetDwmExtend();
                    m_pViewer->Resize(client.right - client.left, client.bottom - client.top);
                    m_pViewer->Show();
                }
            }
            else {
                if (m_pViewer) {
                    m_pViewer->Hide();
                    m_pViewer->ResetDwmExtend();
                }
                m_pDlgTrans->Show();
            }
        }
        else if (pt.x > 0 && pt.x < 34 && pt.y > 234 && pt.y < 266) {
            // 정보
            m_pDlgInfo = new CDlgInfo(m_hInst);
            m_pDlgInfo->Create(hWnd);
        }
        else {
            if (m_pViewer && !m_isTransWinActive) {
                LRESULT lResult = m_pViewer->ProcessMessage(message, wParam, lParam);
            }
            return 0;
        }
    }
    break;
    case WM_SIZE:
    {
        int width = LOWORD(lParam); // 새로운 윈도우의 너비
        int height = HIWORD(lParam); // 새로운 윈도우의 높이

        if (!m_isTransWinActive) {
            if (m_pViewer) {
                m_pViewer->Resize(width, height);

                // 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
                if (m_isMouseInside) {
                    m_pViewer->SetBgOpacity(1);
                    m_pViewer->SetMenuAreaWidth(LEFT_MENU_WIDTH);
                }
                else {
                    float bgOpacity = settings["transparent"] / 100.0f;
                    m_pViewer->SetBgOpacity(bgOpacity);
                    m_pViewer->SetMenuAreaWidth(0);
                }
            }
        }
    }
    break;
    case WM_TIMER: {
        if (wParam == TIMER_ID && !m_isTransWinActive) {
            // 마우스 위치 얻기
            POINT pt;
            GetCursorPos(&pt);

            // 마우스가 창 내부에 있는지 확인
            RECT rc;
            GetWindowRect(hWnd, &rc);
            BOOL isInside = PtInRect(&rc, pt);

            // 상태가 변경되었을 때만 처리
            if (m_isMouseInside != isInside && m_bSetupActive == false) {
                m_isMouseInside = isInside;

                if (m_isMouseInside) {
                    // 마우스가 창 내부로 들어옴
                    // 타이틀바 및 테두리를 추가
                    SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_EX_LAYERED);
                    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
                    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
                    // 메뉴창 표시
                    if (m_pViewer) {
                        m_pViewer->SetMenuAreaWidth(LEFT_MENU_WIDTH);
                    }
                }
                else {
                    // 마우스가 창 외부로 나감
                    SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_LAYERED); // 확장 스타일을 모두 제거
                    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

                    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
                    // 메뉴창 숨김
                    if (m_pViewer) {
                        m_pViewer->SetMenuAreaWidth(0);
                    }
                }
            }
        }
    } break;
    case WM_CLOSE:
    case WM_DESTROY:
        CSettings::GetInstance()->MakeChildCmd("exit", "exit");
        if (m_pBgJob) {
            m_pBgJob->RequestExit();
        }
        // COM 해제
        CoUninitialize();
        // 종료 플래그 설정
        if (m_pBgJob) {
            m_pBgJob->RequestExit();
        }
        PostQuitMessage(0);
        break;
    default:
        if (m_pViewer && !m_isTransWinActive) {
            LRESULT lResult = m_pViewer->ProcessMessage(message, wParam, lParam);
            if (lResult == 0) {
                return 0;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 트램폴린 정보 대화 상자 처리기
INT_PTR CALLBACK RealTrans::StaticAboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    RealTrans* pThis = RealTrans::GetInstance();
    return pThis->AboutProc(hDlg, message, wParam, lParam);
}

// 인스턴스 정보 대화 상자 처리기
INT_PTR RealTrans::AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // 싱글톤 인스턴스 얻기
    RealTrans* pApp = RealTrans::GetInstance();

    // 애플리케이션 실행
    return pApp->Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}