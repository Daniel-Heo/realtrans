#include "CDlgTrans.h"
#include "StrConvert.h"
#include "version.h"
#include <windows.h>
#include <fstream>
#include "CSettings.h"
#include "resource.h"
#include <vector>
#include <thread>
#include "portable-file-dialogs.h"
#include "json.hpp"
#include "RealTrans.h"
#include "Util.h"

using json = nlohmann::json;

#pragma comment(lib, "Comdlg32.lib") // 공통 대화상자 라이브러리 링크
#pragma comment (lib,"gdiplus.lib")

extern std::string addText;
extern json settings;

// GDI+를 초기화하기 위한 변수들
ULONG_PTR           g_gdiplusToken;
GdiplusStartupInput g_gdiplusStartupInput;

// string 형을 WCHAR* 로 변환하는 함수
void StringToWChar(const std::string& s, WCHAR* outStr) {
	// 변환된 문자열의 길이를 계산합니다. 멀티바이트를 와이드 문자로 변환할 때 필요한 길이를 얻습니다.
	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);

	// 실제 변환을 수행합니다.
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, outStr, len);
}

// 클래스 선언
CDlgTrans::CDlgTrans(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
	isTrans = FALSE;
	m_hwnd = NULL;
	m_hwnd = NULL;
	hRich1 = NULL;
	hRich2 = NULL;
	transPercent = 0;
	m_pGdiMenuNormal = nullptr;
	m_pGdiMenuOver = nullptr;
	m_pGdiBarBg = nullptr;
	m_pGdiBar = nullptr;
	window_width = 0;
	window_height = 0;

	GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);
}

// 소멸자 추가
CDlgTrans::~CDlgTrans() {
	// GDI+ 이미지 객체 해제
	if (m_pGdiMenuNormal) {
		delete m_pGdiMenuNormal;
		m_pGdiMenuNormal = nullptr;
	}
	if (m_pGdiMenuOver) {
		delete m_pGdiMenuOver;
		m_pGdiMenuOver = nullptr;
	}
	if (m_pGdiBarBg) {
		delete m_pGdiBarBg;
		m_pGdiBarBg = nullptr;
	}
	if (m_pGdiBar) {
		delete m_pGdiBar;
		m_pGdiBar = nullptr;
	}

	GdiplusShutdown(g_gdiplusToken);
}

void CDlgTrans::InitDialogControls() {
	int index;
	TCHAR buffer[256];    // 문자열을 저장할 버퍼
	HWND sel;

	// 설정값이 없을 경우 기본값 설정 : config.json이 있는 상태에서 default 값이 없으면 이 루틴이 필요함.
	if (settings.value("txt_trans_src", "") == "") {
		settings["txt_trans_src"] = "eng_Latn";
	}
	if (settings.value("txt_trans_tgt", "") == "") {
		settings["txt_trans_tgt"] = "kor_Hang";
	}

	LoadString(hInstance, IDS_TRANSLATE, buffer, 256);

	RECT client;
	GetClientRect(m_hwnd, &client);

	// 번역	버튼
	CreateWindow(
		L"BUTTON", buffer,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
		43, 0, 200, 24,
		m_hwnd, (HMENU)IDC_BTN_TRANSLATE, GetModuleHandle(nullptr), nullptr
	);
	// 파일 열기 버튼
	CreateWindow(
		L"BUTTON", L"File",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
		243, 0, 100, 24,
		m_hwnd, (HMENU)IDC_BTN_OPEN_FILE, GetModuleHandle(nullptr), nullptr
	);
	// 음성 언어 선택 콤보박스
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_CLIPCHILDREN,
		343, 0, 150, 24,
		m_hwnd, (HMENU)IDC_CB_SRCLANG, GetModuleHandle(nullptr), nullptr
	);
	// '->'를 나타내는 static control 추가
	CreateWindow(
		L"STATIC", L"->",
		WS_CHILD | WS_VISIBLE | SS_CENTER | WS_CLIPCHILDREN,
		493, 0, 20, 24,
		m_hwnd, (HMENU)IDC_STATIC_ARROW, GetModuleHandle(nullptr), nullptr
	);

	// 번역 언어 선택 콤보박스
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_CLIPCHILDREN,
		513, 0, 150, 24,
		m_hwnd, (HMENU)IDC_CB_TGTLANG, GetModuleHandle(nullptr), nullptr
	);

	// 파일 저장 버튼
	CreateWindow(
		L"BUTTON", L"Save",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
		663, 0, 100, 24,
		m_hwnd, (HMENU)IDC_BTN_SAVE_FILE, GetModuleHandle(nullptr), nullptr
	);

	// 언어와 관련 데이터 쌍을 배열로 초기화
	LanguageData languages[] = {
		{L"Korean", L"kor_Hang"}, {L"English", L"eng_Latn"},
		{L"German", L"deu_Latn"}, {L"French", L"fra_Latn"}, {L"Italian", L"ita_Latn"},
		{L"Arabic", L"kas_Arab"}, {L"Hindi", L"hin_Deva"}, {L"Polish", L"pol_Latn"},
		{L"Russian", L"rus_Cyrl"}, {L"Turkish", L"tur_Latn"}, {L"Dutch", L"nld_Latn"},
		{L"Spanish", L"spa_Latn"}, {L"Thai", L"tha_Thai"}, {L"Portuguese", L"por_Latn"},
		{L"Indonesian", L"ind_Latn"}, {L"Swedish", L"swe_Latn"}, {L"Czech", L"ces_Latn"},
		{L"Romanian", L"ron_Latn"}, {L"Catalan", L"cat_Latn"}, {L"Hungarian", L"hun_Latn"},
		{L"Ukrainian", L"ukr_Cyrl"}, {L"Greek", L"ell_Grek"}, {L"Bulgarian", L"bul_Cyrl"},
		{L"Serbian", L"srp_Cyrl"}, {L"Macedonian", L"mkd_Cyrl"}, {L"Latvian", L"lav_Latn"},
		{L"Slovenian", L"slv_Latn"}, {L"Galician", L"glg_Latn"}, {L"Danish", L"dan_Latn"},
		{L"Urdu", L"urd_Arab"}, {L"Slovak", L"slk_Latn"}, {L"Hebrew", L"heb_Hebr"},
		{L"Finnish", L"fin_Latn"}, {L"Azerbaijani Latn", L"azj_Latn"},
		{L"Azerbaijani Arab", L"azb_Arab"}, {L"Lithuanian", L"lit_Latn"},
		{L"Estonian", L"est_Latn"}, {L"Nynorsk", L"nno_Latn"}, {L"Welsh", L"cym_Latn"},
		{L"Punjabi Guru", L"pan_Guru"}, {L"Basque", L"eus_Latn"},
		{L"Vietnamese", L"vie_Latn"}, {L"Bengali", L"ben_Beng"}, {L"Nepali", L"npi_Deva"},
		{L"Marathi", L"mar_Deva"}, {L"Belarusian", L"bel_Cyrl"}, {L"Kazakh", L"kaz_Cyrl"},
		{L"Armenian", L"hye_Armn"}, {L"Swahili", L"swh_Latn"}, {L"Tamil", L"tam_Taml"},
		{L"Albanian", L"sqi_Latn"}, {L"Mandarin(TW)", L"zho_Hant"},
		{L"Cantonese(HK)", L"yue_Hant"}, {L"Mandarin(CN)", L"zho_Hans"},
		{L"Cantonese(CN)", L"zho_Hant"}, {L"Japanese", L"jpn_Jpan"}
	};
	int lenLang = sizeof(languages) / sizeof(languages[0]);

	// 원본 텍스트 박스
	HWND hListBox = GetDlgItem(m_hwnd, IDC_CB_SRCLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
			if (CompareStringWString(settings.value("txt_trans_src", ""), (WCHAR*)languages[i].data)) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)index, 0);
			}
		}
	}

	// 출력 텍스트 박스
	hListBox = GetDlgItem(m_hwnd, IDC_CB_TGTLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
			if (CompareStringWString(settings.value("txt_trans_tgt", ""), (WCHAR*)languages[i].data)) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)index, 0);
			}
		}
	}

	// 폭 : 1280-43 = 1237, 1237/2 = 618
	// 높이 : 400-34-5 = 361
	// richedit 컨트롤 추가 좌우 2개
	int width = (client.right - client.left - 43)/2;
	int height = client.bottom - client.top - 24;
	hRich1 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | WS_BORDER | WS_CLIPCHILDREN,
		43, 24, width, height, m_hwnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

	hRich2 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | WS_BORDER | WS_CLIPCHILDREN,
		43+width, 24, width, height, m_hwnd, (HMENU)1003, GetModuleHandle(NULL), NULL);

	// 배경색 변경
	SendMessage(hRich1, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(5, 5, 5));
	SendMessage(hRich2, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(5, 5, 5));

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR; // 폰트 크기와 색상 변경을 나타냅니다.
	cf.yHeight = 12 * 20; // 포인트 단위의 폰트 크기를 트윕스점으로 변환
	cf.crTextColor = RGB(200, 200, 255);; // 원하는 색상을 RGB 값으로 설정합니다.

	// 폰트 색상 변경을 적용합니다.
	SendMessage(hRich1, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SendMessage(hRich2, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

	// RichEdit 컨트롤을 Z-순서에서 맨 위로 이동
	//SetWindowPos(hRich1, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	//SetWindowPos(hRich2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// GDI 초기화
	LoadImages(); // 이미지 로드
}

void CDlgTrans::ShowDialogControls() {
	ShowWindow(hRich1, SW_SHOW);
	ShowWindow(hRich2, SW_SHOW);
	// 버튼 보이기
	
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_TRANSLATE), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_OPEN_FILE), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CB_SRCLANG), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CB_TGTLANG), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_STATIC_ARROW), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SAVE_FILE), SW_SHOW);
}

void CDlgTrans::HideDialogControls() {
	ShowWindow(hRich1, SW_HIDE);
	ShowWindow(hRich2, SW_HIDE);
	// 버튼 숨기기
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_TRANSLATE), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_OPEN_FILE), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CB_SRCLANG), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CB_TGTLANG), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_STATIC_ARROW), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SAVE_FILE), SW_HIDE);

	// progress bar 삭제
	RECT invalidRect = { 43, 5, 1203, 29 };  // 다시 그릴 영역
	InvalidateRect(m_hwnd, &invalidRect, FALSE);  // 무효화하여 다시 그리기 요청
}

// RichEdit 컨트롤을 생성하고 초기화하는 함수
void CDlgTrans::Create(HWND hWndP) {
	if (bExistDlg == FALSE) {
		RECT client;
		GetClientRect(hWndP, &client);
		 
		// D2D 렌더링을 위한 별도 차일드 윈도우 생성
		m_hwnd = hWndP;

		// 컨트롤 초기화
		InitDialogControls();
		bExistDlg = TRUE;
	}
}

void CDlgTrans::Destroy() {
	if (bExistDlg == TRUE) {
		bExistDlg = FALSE;
	}
}

// RichEdit 컨트롤을 숨김
void CDlgTrans::Hide() {
	HideDialogControls();
	bVisible = FALSE;
}

// RichEdit 컨트롤을 숨김
void CDlgTrans::Show() {
	ShowDialogControls();
	bVisible = TRUE;
	// 창을 다시 그리도록 함
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
}

// rich1의 텍스트를 가져옴
void CDlgTrans::GetText(std::string& strOut, std::string& srcLang, std::string& tgtLang) {
	LPARAM itemData;
	// 텍스트 길이를 가져온다.
	int nLen = GetWindowTextLength(hRich1);
	// 텍스트 길이만큼 버퍼를 할당한다.
	WCHAR* pBuf = new WCHAR[nLen + 1];
	// 버퍼에 텍스트를 복사한다.
	GetWindowText(hRich1, pBuf, nLen + 1);
	// 버퍼의 내용을 std::string으로 변환한다.
	strOut = WStringToString(pBuf);

	LRESULT nSel = 0;
	HWND hCbBox;

	hCbBox = GetDlgItem(m_hwnd, IDC_CB_SRCLANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		srcLang = WCHARToString((WCHAR*)itemData);
	}

	hCbBox = GetDlgItem(m_hwnd, IDC_CB_TGTLANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		tgtLang = WCHARToString((WCHAR*)itemData);
	}
}

void GetWindowSize(HWND m_hwnd, int& width, int& height) {
	RECT rect;
	if (GetWindowRect(m_hwnd, &rect)) {
		width = rect.right - rect.left;   // 너비 계산
		height = rect.bottom - rect.top;  // 높이 계산
	}
	else {
		// GetWindowRect 실패 시
		width = 10;
		height = 10;
	}
}

void CDlgTrans::ProgressBar(int percent) {
	static int oldPer = 0;
	if (percent != oldPer) {
		transPercent = percent;
		RECT invalidRect = { 765, 0, 1165, 24 };  // 다시 그릴 영역
		InvalidateRect(m_hwnd, &invalidRect, TRUE);  // 무효화하여 다시 그리기 요청
		if (percent == 100) isTrans = FALSE;
		oldPer = percent;
	}
}

// 이미지 로드 함수
BOOL CDlgTrans::LoadImages() {
	// 이미지 파일 경로 설정
	std::wstring normalImagePath = StringToWString(RealTrans::GetInstance()->GetExecutePath()) + L"lmenu_normal.png";
	std::wstring overImagePath = StringToWString(RealTrans::GetInstance()->GetExecutePath()) + L"lmenu_over.png";
	std::wstring barBgImgPath = StringToWString(RealTrans::GetInstance()->GetExecutePath()) + L"progressbar_silver.png";
	std::wstring barImgPath = StringToWString(RealTrans::GetInstance()->GetExecutePath()) + L"progressbar.png";

	// 이미지 로드
	bool success = true;

	// 일반 메뉴 이미지 로드
	m_pGdiMenuNormal = new Image(normalImagePath.c_str());
	if (m_pGdiMenuNormal->GetLastStatus() != Ok) {
		TRACE(L"GDI+ 일반 메뉴 이미지 로드 실패: %d\n", m_pGdiMenuNormal->GetLastStatus());
		success = false;
	}

	// 오버 메뉴 이미지 로드
	m_pGdiMenuOver = new Image(overImagePath.c_str());
	if (m_pGdiMenuOver->GetLastStatus() != Ok) {
		TRACE(L"GDI+ 오버 메뉴 이미지 로드 실패: %d\n", m_pGdiMenuOver->GetLastStatus());
		success = false;
	}

	// 프로그레스바 배경 이미지 로드
	m_pGdiBarBg = new Image(barBgImgPath.c_str());
	if (m_pGdiBarBg->GetLastStatus() != Ok) {
		TRACE(L"GDI+ 프로그레스바 배경 이미지 로드 실패: %d\n", m_pGdiBarBg->GetLastStatus());
		success = false;
	}

	// 프로그레스바 이미지 로드
	m_pGdiBar = new Image(barImgPath.c_str());
	if (m_pGdiBar->GetLastStatus() != Ok) {
		TRACE(L"GDI+ 프로그레스바 이미지 로드 실패: %d\n", m_pGdiBar->GetLastStatus());
		success = false;
	}

	// 로드 성공 여부 반환
	return success;
}

// #c8c8c8 색상을 COLORREF로 변환
COLORREF CDlgTrans::HexToCOLORREF(const std::string& hexCode) {
	// 색상 코드에서 '#' 문자를 제거합니다.
	std::string hex = hexCode.substr(1);

	// 16진수 값을 10진수로 변환합니다.
	int r = std::stoi(hex.substr(0, 2), nullptr, 16);
	int g = std::stoi(hex.substr(2, 2), nullptr, 16);
	int b = std::stoi(hex.substr(4, 2), nullptr, 16);

	// COLORREF 값으로 변환합니다. RGB 매크로를 사용합니다.
	return RGB(r, g, b);
}

// OnPaint 함수 구현 - D2Render 사용
void CDlgTrans::OnPaint() {
	TRACE(L"DlgTrans OnPaint\n");
	PAINTSTRUCT ps;

	HDC hdc = BeginPaint(m_hwnd, &ps);

	// GDI
	Graphics graphics(hdc);

	RECT client;
	GetClientRect(m_hwnd, &client);

	// 왼쪽 메뉴 영역
	int width = client.right - client.left;
	int height = client.bottom - client.top;
	HBRUSH hBrush = CreateSolidBrush(HexToCOLORREF(settings["bg_color"])); // 또는 settings["bg_color"]에서 변환된 값
	RECT menuRect = { 0, 0, 43, height };
	FillRect(hdc, &menuRect, hBrush);
	if (width > 763) {
		menuRect = { 763, 0, width, 24 };
		FillRect(hdc, &menuRect, hBrush);
	}
	DeleteObject(hBrush);  // 브러시 삭제 필수

	// 번역진행 표시 영역
	if (RealTrans::GetInstance()->GetStopMenu() == FALSE) {
		graphics.DrawImage(m_pGdiMenuNormal, 0, 0, m_pGdiMenuNormal->GetWidth(), m_pGdiMenuNormal->GetHeight());
	}
	else {
		graphics.DrawImage(m_pGdiMenuOver, 0, 0, m_pGdiMenuOver->GetWidth(), m_pGdiMenuOver->GetHeight());
	}

	// 번역상태 표시
	if (isTrans == TRUE) {
		graphics.DrawImage(m_pGdiBarBg, 765, 0, m_pGdiBarBg->GetWidth(), m_pGdiBarBg->GetHeight());

		// 번역 진행 %
		int width = m_pGdiBar->GetWidth() * transPercent / 100;
		graphics.DrawImage(m_pGdiBar, 765, 0, width, m_pGdiBar->GetHeight());
	}

	// 7. 정리
	EndPaint(m_hwnd, &ps);
}

void CDlgTrans::ReplaceText(HWND hEdit, std::wstring txt) {
	SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)txt.c_str());
}

// 메시지 트레이스용 (경고창)
void CDlgTrans::Alert(const WCHAR* msg) {
	MessageBox(m_hwnd, msg, L"Alert", MB_OK);
}

// 요약 버튼 클릭시 백그라운드로 요약
BOOL CDlgTrans::WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int no=0;
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		return (INT_PTR)TRUE;
	}
	case WM_PAINT:
	{
		// D2Render를 사용하여 OnPaint 함수 호출
		OnPaint();
		return 0;
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam); // 메뉴 선택, 버튼 클릭 등의 컨트롤 식별자
		int wmEvent = HIWORD(wParam); // 알림 코드
		std::string strTmp, srcLang, tgtLang;

		// 원래 클라이언트 영역을 저장 (무효화 영역 최소화를 위해)
		RECT originalRect;
		GetClientRect(m_hwnd, &originalRect);

		switch (wmId) {
		case IDCANCEL:
		case IDOK:
			return (INT_PTR)TRUE;
			// 번역 버튼 클릭시에 번역을 수행
		case IDC_BTN_TRANSLATE:
			{
				isTrans = TRUE;
				ProgressBar(10); // 시작시 10%
				// 오른쪽 에디터 데이터 삭제
				ReplaceText(hRich2, L"");
				// 넘겨줄 인자 데이터 생성
				GetText(strTmp, srcLang, tgtLang);
				// 번역을 수행한다.
				Translate(strTmp, srcLang, tgtLang);
			}
			break;
		case IDC_BTN_OPEN_FILE:
			// 파일을 열어서 텍스트를 읽어들인다.
			InputFile();
			break;
		case IDC_BTN_SAVE_FILE:
			OutputFile();
			break;
		case IDC_CB_SRCLANG:
			if (wmEvent == CBN_SELCHANGE) {
				// 콤보박스에서 선택한 항목의 인덱스를 가져옵니다.
				int nSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				// 콤보박스에서 선택한 항목의 데이터를 가져옵니다.
				LRESULT itemData = SendMessage((HWND)lParam, CB_GETITEMDATA, nSel, 0);
				if (itemData != CB_ERR) {
					// 데이터를 문자열로 변환하여 출력합니다.
					settings["txt_trans_src"] = WCHARToString((WCHAR*)itemData);
					CSettings::GetInstance()->SaveSimpleSettings();
				}
			}
			break;
		case IDC_CB_TGTLANG:
			if (wmEvent == CBN_SELCHANGE) {
				// 콤보박스에서 선택한 항목의 인덱스를 가져옵니다.
				int nSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				// 콤보박스에서 선택한 항목의 데이터를 가져옵니다.
				LRESULT itemData = SendMessage((HWND)lParam, CB_GETITEMDATA, nSel, 0);
				if (itemData != CB_ERR) {
					// 데이터를 문자열로 변환하여 출력합니다.
					settings["txt_trans_tgt"] = WCHARToString((WCHAR*)itemData);
					CSettings::GetInstance()->SaveSimpleSettings();
				}
			}
			break;
		}
	}
	break;
	case WM_ERASEBKGND:
		return TRUE; // 배경 지우기를 막음
	case WM_SIZE:
	{
		int width, height;
		window_width = LOWORD(lParam); // 새로운 윈도우의 너비
		window_height = HIWORD(lParam); // 새로운 윈도우의 높이
		width = (window_width - 43) / 2; // 윈도우의 너비에서 여백을 뺀 값
		height = window_height - 24; // 윈도우의 높이에서 여백을 뺀 값

		// 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
		MoveWindow(hRich1, 43, 24, width, height, TRUE);
		MoveWindow(hRich2, 43 + width, 24, width, height, TRUE);
	}
	break;

	case WM_CLOSE:
		return (INT_PTR)TRUE;

	case WM_DESTROY:
		// 필요한 정리 작업 수행
		return 0;
	}
	return (INT_PTR)FALSE;
}

/*----------------------------------------
 * 파일 관련
 ----------------------------------------*/

BOOL CDlgTrans::BrowseFile(WCHAR* szPath, WCHAR* szFile)
{
	auto selection = pfd::open_file("Select a file", ".",
		{ "Text File", "*.txt",
		  "All Files", "*" },
		pfd::opt::multiselect).result();

	if (selection.size() == 0) return FALSE;
	std::string firstFile = selection[0]; // 첫 번째 파일 경로
	if (firstFile.length() > 0)
		StringToWChar(firstFile, szFile);
	else return FALSE;

	return TRUE;
}

// 파일 저장 대화상자를 통해 파일 이름을 선택하는 함수
BOOL CDlgTrans::BrowseSaveFile(WCHAR* szPath, WCHAR* szFile) {
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL; // 부모 윈도우 핸들
	ofn.lpstrFile = szFile; // 파일 이름 버퍼
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = TEXT("Text File\0*.txt\0All File\0*.*\0"); // 필터 설정
	ofn.nFilterIndex = 1; // 기본 필터 선택
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = szPath; // 초기 디렉토리
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // 플래그 설정

	// 파일 저장 대화상자 열기
	if (GetSaveFileName(&ofn) == TRUE) {
		return TRUE;
	}

	return FALSE;
}

BOOL WriteToFile(const std::wstring& fileName, const std::string& content) {
	FILE* file = nullptr;

	// _wfopen_s를 사용하여 유니코드 파일명으로 파일 열기
	errno_t err = _wfopen_s(&file, fileName.c_str(), L"wb");  // 바이너리 모드로 열기
	if (err != 0 || file == nullptr) {
		throw std::runtime_error("Failed to open file with wstring filename");
	}

	// std::ofstream을 사용하여 파일에 멀티바이트 데이터 작성
	std::ofstream ofs(file);  // C++ 스트림으로 연결
	ofs.write(content.c_str(), content.size());  // 데이터 작성

	ofs.close();  // 스트림 닫기
	fclose(file);  // 파일 닫기

	return TRUE;
}

BOOL ReadToFile(const std::wstring& fileName, std::string& content) {
	FILE* file = nullptr;
	errno_t err = _wfopen_s(&file, fileName.c_str(), L"rb"); // 파일을 바이너리 모드로 엽니다.

	if (err != 0 || file == nullptr) {
		// 파일을 열지 못한 경우, 오류를 반환합니다.
		return FALSE;
	}

	// 파일 크기를 얻기 위해 파일 포인터를 끝으로 이동
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	rewind(file);

	// 파일 내용을 읽기 위해 벡터 사용
	std::vector<char> buffer(fileSize);
	size_t bytesRead = fread(buffer.data(), 1, fileSize, file);

	// \r은 삭제 \n은 \r\n으로 
	std::vector<char> bufferOut(fileSize * 2);
	size_t pos = 0;
	for (size_t i = 0; i < bytesRead; i++) {
		if (buffer[i] == '\r') {
		}
		else if (buffer[i] == '\n') {
			bufferOut[pos++] = '\r';
			bufferOut[pos++] = '\n';
		}
		else {
			bufferOut[pos++] = buffer[i];
		}
	}
	// bufferOut 사이즈를 pos로 변경
	bufferOut.resize(pos);

	if (bytesRead != fileSize) {
		// 읽은 데이터가 파일 크기와 일치하지 않는 경우
		fclose(file);
		return FALSE;
	}

	// 파일 내용을 문자열로 변환
	content = std::string(bufferOut.begin(), bufferOut.end());

	// 파일 닫기
	fclose(file);

	return TRUE;
}

// 파일 저장
BOOL CDlgTrans::OutputFile()
{
	// 파일 선택 대화상자를 표시한다.
	WCHAR strFileName[MAX_PATH] = { 0 };
	memset(strFileName, 0, sizeof(strFileName));
	WCHAR strPath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, strPath, MAX_PATH);
	if (!BrowseSaveFile(strPath, strFileName)) return FALSE;
	std::wstring wstrFileName = strFileName;

	// rich2의 텍스트를 가져온다.
	int text_length = SendMessage(hRich2, WM_GETTEXTLENGTH, 0, 0); // 텍스트 길이 가져오기
	WCHAR* pBuf = new WCHAR[text_length + 1];
	SendMessage(hRich2, WM_GETTEXT, text_length + 1, (LPARAM)pBuf);
	std::string strOut = WCHARToString(pBuf);

	WriteToFile(wstrFileName, strOut);

	return TRUE;
}

BOOL CDlgTrans::InputFile()
{
	// 영상파일 선택
	WCHAR strFileName[MAX_PATH];
	memset(strFileName, 0, sizeof(strFileName));
	WCHAR strPath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, strPath, MAX_PATH);
	if (!BrowseFile(strPath, strFileName)) return FALSE;

	std::wstring wstrFileName = strFileName;

	std::string strText = "";
	ReadToFile(wstrFileName, strText);

	// rich1에 텍스트를 쓴다.
	if (hRich1 != NULL && strText.length() > 0) {
		ReplaceText(hRich1, StringToWString(strText));
	}

	return TRUE;
}

// 번역을 수행하는 함수
void CDlgTrans::Translate(std::string& strOut, std::string& srcLang, std::string& tgtLang) {
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + "translate.txt";
	std::ofstream outFile(strFileName);
	outFile << srcLang << " -> " << tgtLang << std::endl;
	outFile << strOut;
	outFile.close();
}

void CDlgTrans::CheckTransFile() {
	// 파일을 읽어들인다.
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + "translate_out.txt";
	std::ifstream inFile(strFileName);
	// 파일이 없으면 리턴
	if (!inFile.is_open()) {
		return;
	}

	SETTEXTEX st;
	st.flags = ST_DEFAULT; // 기본 설정 사용
	st.codepage = 1200;    // UTF-16LE 코드 페이지
	DWORD start = 0, end = 0;

	// 파일을 읽어들인다.
	std::string strTmp;
	while (std::getline(inFile, strTmp)) {
		// rich2에 텍스트를 쓴다.
		if (hRich2 != NULL) {
			SendMessage(hRich2, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

			// 커서 위치(현재 선택의 끝)를 얻습니다.
			SendMessage(hRich2, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			SendMessage(hRich2, EM_REPLACESEL, (WPARAM)&st, (LPARAM)StringToWString(strTmp).c_str());
		}
	}
	// 파일을 닫는다.
	inFile.close();

	// 파일을 삭제한다.
	remove(strFileName.c_str());
}