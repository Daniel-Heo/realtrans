// 요약해주는 richedit dialogue control class
#include "CDlgSummary.h"
#include "json.hpp" // 또는 경로를 지정해야 할 경우: #include "External/json.hpp"
#include "openai_api.h"
#include "StrConvert.h"

#define MAX_RICHEDIT_TEXT 10240

// nlohmann 라이브러리의 네임스페이스 사용  
using json = nlohmann::json;

// 환경 설정을 위한 JSON 객체 생성
extern json settings;
extern CDlgSummary* DlgSum;
extern HWND hwndTextBox;
extern std::string strEng;
extern bool runSummary;

//WCHAR rich_buf[MAX_RICHEDIT_TEXT];
std::wstring rich_buf;
bool addSummaryFlag =false;

// int 값을 요약창에 보이게
void DebugInt(HWND hEdit, int value)
{
	WCHAR buf[200];
	wsprintf(buf, L"int: [%d]", value);
	SendMessage(hEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
}

// 요약 버튼 클릭시 백그라운드로 요약
INT_PTR CALLBACK SummaryProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		//case WM_CREATE:
	case WM_INITDIALOG:
	{
		// 다이얼로그 초기화 코드 시작
		// RichEdit 컨트롤 생성
		DlgSum->hSumRichEdit = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
			10, 45, 824, 235, hwndDlg, NULL, DlgSum->hInstance, NULL);

		if (!DlgSum->hSumRichEdit) {
			MessageBox(DlgSum->hWndParent, TEXT("Error to create RichEdit control."), TEXT("오류"), MB_OK);
		}
		else {
			HFONT hFont;
			// RichEdit 컨트롤 초기화
			// 폰트 생성
			hFont = CreateFont(20, // -MulDiv(20, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72), // 높이
				10,                          // 너비
				0,                          // 회전 각도
				0,                          // 기울기 각도
				FW_NORMAL, // FW_NORMAL,                  // 폰트 굵기
				FALSE,                      // 이탤릭
				FALSE,                      // 밑줄
				FALSE,                      // 취소선
				ANSI_CHARSET, //HANGUL_CHARSET, //ANSI_CHARSET,               // 문자 집합
				OUT_TT_PRECIS, //OUT_DEFAULT_PRECIS,         // 출력 정밀도
				CLIP_DEFAULT_PRECIS,        // 클리핑 정밀도
				DEFAULT_QUALITY,            // 출력 품질
				DEFAULT_PITCH | FF_ROMAN, //| FF_DONTCARE, //DEFAULT_PITCH | FF_SWISS,   // 피치와 패밀리
				TEXT("Arial"));             // 폰트 이름

			// 폰트 적용
			SendMessage(DlgSum->hSumRichEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

			// 배경색 변경
			SendMessage(DlgSum->hSumRichEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));

			DlgSum->SetFont();


		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG: // 다이얼로그의 배경색을 변경
		return (INT_PTR)CreateSolidBrush(RGB(0, 0, 0));

	case WM_PAINT:
	{

	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam); // 메뉴 선택, 버튼 클릭 등의 컨트롤 식별자
		int wmEvent = HIWORD(wParam); // 알림 코드

		switch (wmId) {
		case IDCANCEL:
		{
			DlgSum->Hide();
			return (INT_PTR)TRUE;
		}
		break;
		case IDOK:
		{
			return TRUE;
		}
		break;
		}

		switch (wmEvent) {
		case BN_CLICKED:
			switch (wmId) {
			case IDC_SUM:
			{
				// api 키 존재여부 확인
				if (settings.value("ed_summary_api_key", "") == "") {
					MessageBox(hwndDlg, L"API Key is None.", L"Warning", MB_OK);
					return FALSE;
				}
				// 처리할 내용이 있는지 확인
				if (strEng.size() <= DlgSum->nParentRichPos) {
					MessageBox(hwndDlg, L"Summary text is None.", L"Warning", MB_OK);
					return FALSE;
				}
				//DlgSum->MakeSummary();
				runSummary = true;
				return (INT_PTR)TRUE;
			}
			break;

			}
			break;
		}
	}
	break;
	case WM_SIZE:
	{
		int nWidth = LOWORD(lParam); // 새로운 너비
		int nHeight = HIWORD(lParam); // 새로운 높이

		SetWindowPos(DlgSum->hSumRichEdit, NULL, 10, 45, nWidth - 20, nHeight - 55, SWP_NOZORDER);
	}
	break;
	case WM_CLOSE:
		DlgSum->Hide();
		return (INT_PTR)TRUE;
	case WM_DESTROY:
		// 필요한 정리 작업 수행
		return 0;
	}
	return (INT_PTR)FALSE;
}

// 클래스 선언
CDlgSummary::CDlgSummary(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
	nParentRichPos = 0;
}

// RichEdit 컨트롤을 생성하고 초기화하는 함수
void CDlgSummary::Create(HWND hWndP) {
	if (bExistDlg == FALSE) {
		hWndParent = hWndP;
		// Modaless Dialogue 생성
		hDlgSummary = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_SUMMARY), hWndParent, SummaryProc);
		SetWindowLong(hDlgSummary, GWL_EXSTYLE, GetWindowLong(hDlgSummary, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hDlgSummary, 0, (255 * 80) / 100, LWA_ALPHA);
		bExistDlg = TRUE;
	}

	if (bVisible == FALSE) {
		Show();
	}
}

// RichEdit 컨트롤에 텍스트 설정
void CDlgSummary::SetText(const wchar_t* text) {
	SETTEXTEX st;
	st.flags = ST_DEFAULT; // 기본 설정 사용
	st.codepage = 1200;    // UTF-16LE 코드 페이지
	SendMessage(hSumRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)text);
}

// RichEdit 컨트롤에서 텍스트 가져오기
void CDlgSummary::GetText(wchar_t* buffer, int bufferSize) {
	SendMessage(hSumRichEdit, WM_GETTEXT, (WPARAM)bufferSize, (LPARAM)buffer);
}

// RTF 형식의 1x2 테이블을 생성하여 RichEdit 컨트롤에 삽입하는 함수
void CDlgSummary::InsertTable(const std::wstring& text) {
	// 셀 폭을 계산
	HDC hdc = GetDC(hSumRichEdit);
	int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(hSumRichEdit, hdc);

	RECT rect;
	GetClientRect(hSumRichEdit, &rect);
	int widthInPixels = rect.right - rect.left;

	int cellWidth = static_cast<int>((widthInPixels * 1440) / dpiX);
	wchar_t buffer[256]; // 임시 버퍼
	std::wstring in_buf;
	wsprintfW((LPWSTR)buffer, L"\\cellx%d ", cellWidth);
	in_buf = buffer;

	// RTF 형식의 1x2 테이블 정의
	// 여기서는 테이블의 각 셀의 너비를 조절할 수 있습니다. \cellxN은 N 위치에 셀을 끝내는 것을 의미합니다.
	std::wstring rtfTable = 
		L"{\\rtf1"
		L"\\trowd\\trgaph108\\trleft-108"
		+ in_buf + // 1행 1열의 너비를 설정합니다.
		L"\\intbl "
		+ text + // 첫번째 셀 : 내용 
		L"\\cell"
		L"\\row" // 행을 종료합니다.
		L"}";

	AddText(rtfTable);
	GoBottom();
}

// tokken size 체크
int CDlgSummary::getTokenSize(const std::string& wstr, int curPos)
{
	/*
		GPT-3.5 Turbo: GPT-3.5 Turbo는 주로 비용 효율적인 선택지로 사용되며, 대체로 4096 토큰의 제한을 가지고 있습니다.
		이는 요청(payload)과 응답을 합친 전체 길이가 4096 토큰을 넘지 않아야 함을 의미합니다.

		GPT-4.0: 최대 토큰 제한은 약 8000~16384 토큰 범위 내에서 모델 및 사용 설정에 따라 달라질 수 있습니다.
		이는 GPT-4가 처리할 수 있는 입력과 생성할 수 있는 출력의 총 합계 길이에 대한 제한입니다.
	*/
	int cnt = 0;
	for (char wc : wstr) {
		if (wc == ' ') {
			cnt++;
		}
	}
	cnt++;

	return cnt;
}

// Handle로 RichEdit 컨트롤에서 텍스트 가져오기
BOOL CDlgSummary::GetTextWithHandle(std::string& buf) {
	int lenText;

	buf = '\0';
	//lenText = GetTextSize(hRich);
	lenText = strEng.size();
	if (nParentRichPos < lenText) {
		buf = strEng.substr(nParentRichPos, lenText - nParentRichPos).c_str();
		nParentRichPos = lenText;

		// 추가된 것이 없을 경우
		if (buf.size() == 0) return false;

		return true;
	}

	return false;
}

// RichEdit 컨트롤에 텍스트 설정
void CDlgSummary::AddText(const std::wstring& text) {
	// 텍스트의 마지막에 커서를 위치시킵니다. -1, -1은 텍스트의 끝을 나타냅니다.
	SendMessage(hSumRichEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	SendMessage(hSumRichEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)text.c_str());
}

// RichEdit 컨트롤에 텍스트 사이즈 가져오기
int CDlgSummary::GetTextSize(HWND hRich) {
	// 텍스트 사이즈 가져오기 ( 유니코드일 경우에 사이즈가 2배일 수 있음)
	int ret = SendMessage(hRich, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0); // WPARAM과 LPARAM은 사용하지 않으므로 0으로 설정합니다.
	return ret;
}

// RichEdit 컨트롤을 숨김
void CDlgSummary::Hide() {
	ShowWindow(hDlgSummary, SW_HIDE);
	bVisible = FALSE;
}

// RichEdit 컨트롤을 숨김
void CDlgSummary::Show() {
	ShowWindow(hDlgSummary, SW_SHOW);
	bVisible = TRUE;
}

// 요약 버튼 클릭시 백그라운드로 요약
BOOL CDlgSummary::BgSummary() {
	// Parent Richedit에서 텍스트 가져오기
	BOOL ret;
	char lang[30];
	std::string req;
	std::wstring strWOut=L"";

	ret = GetTextWithHandle(req);
	if (ret == false) {
		return ret;
	}
	else {
		req=ConvertToUTF8(req);
	}

	// api 키 존재여부 확인
	if (settings.value("ed_summary_api_key", "") == "") {
		MessageBox(hDlgSummary, L"API Key is None.", L"Warning", MB_OK);
		return false;
	}

	// req 사이즈가 10k 초과시 잘라서 처리한다.
	int reqSize = req.size();
	std::string tmpReq;
	size_t pos = 0;

	while (1) {
		if (reqSize > 9192) {
			pos = req.find('\n', 9192);
			if (pos == std::string::npos) {
				pos = req.find(' ', 9192);
				if (pos == std::string::npos) {
					tmpReq = req.substr(0, 9192);
					req = req.substr(9192);
				}
				else {
					tmpReq = req.substr(0, pos);
					req = req.substr(pos);
				}
			}
			else {
				tmpReq = req.substr(0, pos);
				req = req.substr(pos);
			}
		}
		else {
			tmpReq = req;
			req.clear();
		}

		// 20자 이하면 skip
		if (tmpReq.size() < 20) break;

		// API 연동
		// env에서 요약정보 가져오기
		if (settings["cb_summary_api"] == 0) {
			// openai API
			// 언어 설정
			switch(settings["cb_summary_lang"].get<int>()) {
				case 0: strcpy_s(lang, "Korean"); break; // 한국어 ( Korean )
				case 1: strcpy_s(lang, "English"); break; // 영어 ( English )
				case 2: strcpy_s(lang, "German"); break; // 독일어 ( German )
				case 3: strcpy_s(lang, "Spanish"); break; // 스페인어 ( Spanish ) 
				case 4: strcpy_s(lang, "French"); break; // 프랑스어 ( French )
				case 5: strcpy_s(lang, "Italian"); break; // 이탈리아어 ( Italian )
				case 6: strcpy_s(lang, "Japanese"); break; // 일본어 ( Japanese )
				case 7: strcpy_s(lang, "Portuguese"); break; // 포르투갈어 ( Portuguese )
				case 8: strcpy_s(lang, "Russian"); break; // 러시아어 ( Russian )
				case 9: strcpy_s(lang, "Chinese"); break; // 중국어 ( Chinese ) 
				case 10: strcpy_s(lang, "Arabic"); break; // 아랍어 ( Arabic ) 
				case 11: strcpy_s(lang, "Hindi"); break; // 힌디어 ( Hindi ) 
				default: strcpy_s(lang, "English"); break; // 영어 ( English )
			}
			RequestOpenAIChat(tmpReq, strWOut, lang, settings["ed_summary_hint"], StringToWStringInSummary(settings.value("ed_summary_api_key", "")).c_str());
		}

		// 번역
		
		// child Richedit에 텍스트 추가
		rich_buf += L"\r\n\r\n"+strWOut;

		reqSize = req.size();
		if (reqSize <= 0) break;
	}

	addSummaryFlag = true;

	return true;
}

BOOL CDlgSummary::AddRichText() {
	if (addSummaryFlag == true) {
		//InsertTable(rich_buf);
		AddText(rich_buf);
		GoBottom();
		rich_buf.clear();
		addSummaryFlag = false;
	}

	return true;
}

// 메시지 트레이스용 : Trace창으로 사용할 경우 사용
BOOL CDlgSummary::Trace(const std::wstring& trace) {
	AddText(trace+L"\r\n");
	GoBottom();
	return true;
}

// 메시지 트레이스용 (경고창)
void CDlgSummary::Alert(const WCHAR* msg) {
	MessageBox(hDlgSummary, msg, L"Alert", MB_OK);
}

// 요약창의 보여지는 위치를 제일 마지막줄이 보이게 이동
void CDlgSummary::GoBottom() {
	// 커서를 문서의 끝으로 이동시킵니다.
	//SetFocus(hSumRichEdit);
	//SendMessage(hSumRichEdit, EM_SETSEL, -1, 0);
	SendMessage(hSumRichEdit, EM_SCROLL, SB_BOTTOM, 0);
}

// #c8c8c8 색상을 COLORREF로 변환
COLORREF CDlgSummary::HexToCOLORREF(const std::string& hexCode) {
	// 색상 코드에서 '#' 문자를 제거합니다.
	std::string hex = hexCode.substr(1);

	// 16진수 값을 10진수로 변환합니다.
	int r = std::stoi(hex.substr(0, 2), nullptr, 16);
	int g = std::stoi(hex.substr(2, 2), nullptr, 16);
	int b = std::stoi(hex.substr(4, 2), nullptr, 16);

	// COLORREF 값으로 변환합니다. RGB 매크로를 사용합니다.
	return RGB(r, g, b);
}

// 폰트 색상을 변경하는 함수
void CDlgSummary::SetFont() {
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR;
	cf.yHeight = settings["cb_sumfont_size"] * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환
	//SendMessage(hRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

	//cf.dwMask = CFM_COLOR; // 색상 변경을 나타냅니다.
	cf.crTextColor = HexToCOLORREF(settings["ed_sumfont_color"]); // 원하는 색상을 RGB 값으로 설정합니다.

	// 폰트 색상 변경을 적용합니다.
	SendMessage(hSumRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

//SetFocus(hRichEdit);
//SendMessage(hRichEdit, EM_SETSEL, end, -1);
//SetRichEditSelColor(hRichEdit, RGB(200, 200, 255));
//
//// 선택을 취소하고 싶다면 다음과 같이 합니다.
//// 이는 커서를 문서의 끝으로 이동시킨 후 선택 영역이 없도록 합니다.
//SendMessage(hRichEdit, EM_SETSEL, -1, 0);