// 요약해주는 richedit dialogue control class
#include "CDlgTrans.h"
#include "StrConvert.h"
#include "version.h"
#include <windows.h>
#include <fstream>
#include "CSettings.h"
#include "resource.h"
#include <gdiplus.h>
using namespace Gdiplus;

extern CDlgTrans* DlgTrans;
extern BOOL bStopMenu;

// 번역을 수행하는 함수
void CDlgTrans::Translate(std::string& strOut, std::string& srcLang, std::string& tgtLang) {
	// 파일을 만든다.
	std::ofstream outFile("translate.txt");
	// 파일에 쓴다.
	outFile<<srcLang << " -> " << tgtLang << std::endl;
	outFile <<strOut;
	// 파일을 닫는다.
	outFile.close();
	isTrans = TRUE;
}

void CDlgTrans::CheckTransFile() {
	//if( isTrans!=TRUE ) return;

	// 파일을 읽어들인다.
	std::ifstream inFile("translate_out.txt");
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
	remove("translate_out.txt");
}

void GetWindowSize(HWND hWnd, int& width, int& height) {
	RECT rect;
	if (GetWindowRect(hWnd, &rect)) {
		width = rect.right - rect.left;   // 너비 계산
		height = rect.bottom - rect.top;  // 높이 계산
	}
	else {
		// GetWindowRect 실패 시
		width = 0;
		height = 0;
	}
}

// 요약 버튼 클릭시 백그라운드로 요약
BOOL CDlgTrans::WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		CreateWindowEx(0, L"STATIC", APP_TITLE_VERSION, WS_CHILD | WS_VISIBLE | SS_CENTER,
			70, 20, 100, 20, hwndDlg, (HMENU)IDC_STATIC, GetModuleHandle(NULL), NULL);
		return (INT_PTR)TRUE;
	}
	case WM_PAINT:
		// 사각형 그리기
	{
		// 시작점 : 34+10 = 43, 5+24+5 = 34
		// 종료점 : 1280 - 43 - 10 = 1280 - 53 =1227, 400 - 34 -5 = 400 - 39 = 361
		// 중간점 : 1227/2 = 613, [613-5=608]
		// 사각형 그리기 : 43, 34, 600, 310, 648, 34, 600, 310
		// 윈도우 창에서 그래픽 작업을 시작합니다.
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwndDlg, &ps);

		Graphics graphics(hdc);
		if (bStopMenu == FALSE) {
			Image image(L"lmenu_normal.png");
			graphics.DrawImage(&image, 0, 0, image.GetWidth(), image.GetHeight());
		}
		else {
			Image image(L"lmenu_over.png");
			graphics.DrawImage(&image, 0, 0, image.GetWidth(), image.GetHeight());
		}

		// 회색 펜 생성 (회색으로 테두리를 그릴 것임)
		HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
		// 생성한 펜을 디바이스 컨텍스트에 선택
		SelectObject(hdc, hPen);

		// 투명 브러시 생성 (내부를 비워두기 위함)
		HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
		// 생성한 브러시를 디바이스 컨텍스트에 선택
		SelectObject(hdc, hBrush);

		// 회색 테두리와 투명 내부를 가진 사각형 그리기
		GetWindowSize(hwndDlg, window_width, window_height);
		int width = (window_width - 64) / 2; // 윈도우의 너비에서 여백을 뺀 값
		int height = window_height - 42; // 윈도우의 높이에서 여백을 뺀 값
		Rectangle(hdc, 43, 33, 35+width, window_height - 50);
		Rectangle(hdc, 38+width, 33, window_width-29, window_height - 50);

		//MoveWindow(hRich1, 44, 34, width, height, TRUE);
		//MoveWindow(hRich2, 54 + width, 34, width, height, TRUE);

		// 사용이 끝난 GDI 오브젝트 제거
		DeleteObject(hPen);
		DeleteObject(hBrush); // 실제로는 NULL_BRUSH는 시스템 오브젝트이므로 제거하지 않아도 됩니다.

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam); // 메뉴 선택, 버튼 클릭 등의 컨트롤 식별자
		int wmEvent = HIWORD(wParam); // 알림 코드

		switch (wmId) {
		case IDCANCEL:
		case IDOK:
			return (INT_PTR)TRUE;
			break;
		// 번역 버튼 클릭시에 번역을 수행
		case IDC_BTN_TRANSLATE:
			// 넘겨줄 인자 데이터 생성
			std::string strTmp, srcLang, tgtLang;
			GetText(strTmp, srcLang, tgtLang);
			// 번역을 수행한다.
			Translate(strTmp, srcLang, tgtLang);
			break;
		}
	}
	break;
	case WM_SIZE:
	{
		int width, height;
		window_width = LOWORD(lParam); // 새로운 윈도우의 너비
		window_height = HIWORD(lParam); // 새로운 윈도우의 높이
		width = (window_width - 64)/2; // 윈도우의 너비에서 여백을 뺀 값
		height = window_height - 42; // 윈도우의 높이에서 여백을 뺀 값

		// 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
		// 34, 10, rich, 10, rich, 10
		MoveWindow(hRich1, 44, 34, width, height, TRUE);
		MoveWindow(hRich2, 54+width, 34, width, height, TRUE);
	}
	break;

	case WM_CLOSE:
		//EndDialog(hwndDlg, 0);
		//DestroyWindow(hwndDlg);
		return (INT_PTR)TRUE;

	case WM_DESTROY:
		// 필요한 정리 작업 수행
		//EndDialog(hwndDlg, 0);
		//DestroyWindow(hwndDlg);
		return 0;
	}
	return (INT_PTR)FALSE;
}

// 클래스 선언
CDlgTrans::CDlgTrans(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
	isTrans = FALSE;
	hWnd = NULL;
	hRich1 = NULL;
	hRich2 = NULL;
}

void CDlgTrans::InitDialogControls() {
	int index;
	TCHAR buffer[256];    // 문자열을 저장할 버퍼

	LoadString(hInstance, IDS_TRANSLATE, buffer, 256);

	// 번역	버튼
	CreateWindow(
		L"BUTTON", buffer,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		43, 5, 200, 24,
		hWnd, (HMENU)IDC_BTN_TRANSLATE, GetModuleHandle(nullptr), nullptr
	);
	// 음성 언어 선택 콤보박스
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST| WS_VSCROLL,
		253, 5, 150, 24,
		hWnd, (HMENU)IDC_CB_SRCLANG, GetModuleHandle(nullptr), nullptr
	);
	// '->'를 나타내는 static control 추가
	CreateWindow(
		L"STATIC", L"->",
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		403, 5, 20, 24,
		hWnd, (HMENU)IDC_STATIC_ARROW, GetModuleHandle(nullptr), nullptr
	);
	
	// 번역 언어 선택 콤보박스
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST| WS_VSCROLL,
		423, 5, 150, 24,
		hWnd, (HMENU)IDC_CB_TGTLANG, GetModuleHandle(nullptr), nullptr
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
	HWND hListBox = GetDlgItem(hWnd, IDC_CB_SRCLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
		}
		SendMessage(hListBox, CB_SETCURSEL, (WPARAM)1, 0);
	}
	// 출력 텍스트 박스
	hListBox = GetDlgItem(hWnd, IDC_CB_TGTLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
		}
		SendMessage(hListBox, CB_SETCURSEL, (WPARAM)0, 0);
	}

	// 폭 : 1280-43 = 1237, 1237/2 = 618
	// 높이 : 400-34-5 = 361
	// richedit 컨트롤 추가 좌우 2개
	hRich1 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE, // | ES_READONLY,
		44, 34, 590, 310, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

	hRich2 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE, // | ES_READONLY,
		648, 34, 600, 310, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR; // 폰트 크기와 색상 변경을 나타냅니다.
	cf.yHeight = 12 * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환
	cf.crTextColor = RGB(200, 200, 255);; // 원하는 색상을 RGB 값으로 설정합니다.

	// 폰트 색상 변경을 적용합니다.
	SendMessage(hRich1, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SendMessage(hRich2, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

	// 배경색 변경
	SendMessage(hRich1, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));
	SendMessage(hRich2, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));
}

void CDlgTrans::ShowDialogControls() {
	ShowWindow(hRich1, SW_SHOW);
	ShowWindow(hRich2, SW_SHOW);
	// 버튼 보이기
	ShowWindow(GetDlgItem(hWnd, IDC_BTN_TRANSLATE), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_SRCLANG), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_TGTLANG), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARROW), SW_SHOW);
}

void CDlgTrans::HideDialogControls() {
	ShowWindow(hRich1, SW_HIDE);
	ShowWindow(hRich2, SW_HIDE);
	// 버튼 숨기기
	ShowWindow(GetDlgItem(hWnd, IDC_BTN_TRANSLATE), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_SRCLANG), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_TGTLANG), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARROW), SW_HIDE);
}

// RichEdit 컨트롤을 생성하고 초기화하는 함수
void CDlgTrans::Create(HWND hWndP) {
	if (bExistDlg == FALSE) {
		hWnd = hWndP;
		InitDialogControls();
		bExistDlg = TRUE;
	}

	/*if (bVisible == FALSE) {
		Show();
	}*/
}

void CDlgTrans::Destroy() {
	if (bExistDlg == TRUE) {
		//DestroyWindow(hDlg);
		bExistDlg = FALSE;
	}
}

// RichEdit 컨트롤을 숨김
void CDlgTrans::Hide() {
	//ShowWindow(hDlg, SW_HIDE);
	HideDialogControls();
	bVisible = FALSE;
}

// RichEdit 컨트롤을 숨김
void CDlgTrans::Show() {
	//ShowWindow(hDlg, SW_SHOW);
	ShowDialogControls();
	bVisible = TRUE;
}

// rich1의 텍스트를 가져옴
void CDlgTrans::GetText(std::string &strOut, std::string &srcLang, std::string &tgtLang) {
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

	hCbBox = GetDlgItem(hWnd, IDC_CB_SRCLANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		srcLang = WCHARToString((WCHAR*)itemData);
	}

	hCbBox = GetDlgItem(hWnd, IDC_CB_TGTLANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		tgtLang = WCHARToString((WCHAR*)itemData);
	}
}

// 메시지 트레이스용 (경고창)
void CDlgTrans::Alert(const WCHAR* msg) {
	MessageBox(hWnd, msg, L"Alert", MB_OK);
}