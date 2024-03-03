// ������ִ� richedit dialogue control class
#include "CDlgSummary.h"
#include "json.hpp" // �Ǵ� ��θ� �����ؾ� �� ���: #include "External/json.hpp"
#include "openai_api.h"
#include "StrConvert.h"

#define MAX_RICHEDIT_TEXT 10240

// nlohmann ���̺귯���� ���ӽ����̽� ���
using json = nlohmann::json;

// ȯ�� ������ ���� JSON ��ü ����
extern json settings;
extern CDlgSummary* DlgSum;
extern HWND hwndTextBox;
extern std::string strEng;
extern BOOL runSummary;

//WCHAR rich_buf[MAX_RICHEDIT_TEXT];
std::wstring rich_buf;
BOOL addSummaryFlag =false;

// int ���� ���â�� ���̰�
void DebugInt(HWND hEdit, int value)
{
	WCHAR buf[200];
	wsprintf(buf, L"int: [%d]", value);
	SendMessage(hEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
}

// ��� ��ư Ŭ���� ��׶���� ���
INT_PTR CALLBACK SummaryProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		//case WM_CREATE:
	case WM_INITDIALOG:
	{
		// ���̾�α� �ʱ�ȭ �ڵ� ����
		// LoadSettings(hwndDlg, "config.json");
		// RichEdit ��Ʈ�� ����
		DlgSum->hSumRichEdit = CreateWindowEx(0, RICHEDIT_CLASS, TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
			10, 45, 824, 235, hwndDlg, NULL, DlgSum->hInstance, NULL);

		if (!DlgSum->hSumRichEdit) {
			MessageBox(DlgSum->hWndParent, TEXT("RichEdit ��Ʈ���� ������ �� �����ϴ�."), TEXT("����"), MB_OK);
		}
		else {
			HFONT hFont;
			// RichEdit ��Ʈ�� �ʱ�ȭ
			// ��Ʈ ����
			hFont = CreateFont(20, // -MulDiv(20, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72), // ����
				10,                          // �ʺ�
				0,                          // ȸ�� ����
				0,                          // ���� ����
				FW_NORMAL, // FW_NORMAL,                  // ��Ʈ ����
				FALSE,                      // ���Ÿ�
				FALSE,                      // ����
				FALSE,                      // ��Ҽ�
				HANGUL_CHARSET, //ANSI_CHARSET,               // ���� ����
				OUT_TT_PRECIS, //OUT_DEFAULT_PRECIS,         // ��� ���е�
				CLIP_DEFAULT_PRECIS,        // Ŭ���� ���е�
				DEFAULT_QUALITY,            // ��� ǰ��
				DEFAULT_PITCH | FF_ROMAN, //| FF_DONTCARE, //DEFAULT_PITCH | FF_SWISS,   // ��ġ�� �йи�
				TEXT("D2Coding"));             // ��Ʈ �̸�

			// ��Ʈ ����
			SendMessage(DlgSum->hSumRichEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

			// ���� ����
			SendMessage(DlgSum->hSumRichEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));

			// ��Ʈ ������ �� �÷�
			//SetRichEditTextColor2(DlgSum->hSumRichEdit, RGB(200, 200, 255));
			DlgSum->SetFont();


		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG: // ���̾�α��� ������ ����
		return (INT_PTR)CreateSolidBrush(RGB(0, 0, 0));

	case WM_PAINT:
	{

	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam); // �޴� ����, ��ư Ŭ�� ���� ��Ʈ�� �ĺ���
		int wmEvent = HIWORD(wParam); // �˸� �ڵ�

		switch (wmId) {
		case IDCANCEL:
		{
			//EndDialog(hwndDlg, LOWORD(wParam));
			//DestroyWindow(hwndDlg);
			DlgSum->Hide();
			return (INT_PTR)TRUE;
		}
		break;
		case IDOK:
		{
			HWND hFocus = GetFocus();
			if (hFocus == DlgSum->hSumRichEdit) {
				//SendMessage(hSumRichEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)L"OK");
			}
			return TRUE;
		}
		break;
		}

		switch (wmEvent) {
		case BN_CLICKED:
			switch (wmId) {
			case IDC_SUM:
			{
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
		int nWidth = LOWORD(lParam); // ���ο� �ʺ�
		int nHeight = HIWORD(lParam); // ���ο� ����

		SetWindowPos(DlgSum->hSumRichEdit, NULL, 10, 45, nWidth - 20, nHeight - 55, SWP_NOZORDER);
	}
	break;
	case WM_CLOSE:
		//EndDialog(hwndDlg, 0);
		//DestroyWindow(hwndDlg);
		DlgSum->Hide();
		return (INT_PTR)TRUE;
	case WM_DESTROY:
		// �ʿ��� ���� �۾� ����
		return 0;
	}
	return (INT_PTR)FALSE;

	//return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

// Ŭ���� ����
CDlgSummary::CDlgSummary(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
	nParentRichPos = 0;
}

// RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
void CDlgSummary::Create(HWND hWndP) {
	if (bExistDlg == FALSE) {
		hWndParent = hWndP;
		// Modaless Dialogue ����
		hDlgSummary = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_SUMMARY), hWndParent, SummaryProc);
		//ShowWindow(hDlgSummary, SW_SHOW);
		SetWindowLong(hDlgSummary, GWL_EXSTYLE, GetWindowLong(hDlgSummary, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hDlgSummary, 0, (255 * 80) / 100, LWA_ALPHA);
		bExistDlg = TRUE;
	}

	if (bVisible == FALSE) {
		Show();
	}
}

// RichEdit ��Ʈ�ѿ� �ؽ�Ʈ ����
void CDlgSummary::SetText(const wchar_t* text) {
	SendMessage(hSumRichEdit, WM_SETTEXT, 0, (LPARAM)text);
}

// RichEdit ��Ʈ�ѿ��� �ؽ�Ʈ ��������
void CDlgSummary::GetText(wchar_t* buffer, int bufferSize) {
	SendMessage(hSumRichEdit, WM_GETTEXT, (WPARAM)bufferSize, (LPARAM)buffer);
}

// tokken size üũ
int CDlgSummary::getTokenSize(const std::string& wstr, int curPos)
{
	/*
		GPT-3.5 Turbo: GPT-3.5 Turbo�� �ַ� ��� ȿ������ �������� ���Ǹ�, ��ü�� 4096 ��ū�� ������ ������ �ֽ��ϴ�.
		�̴� ��û(payload)�� ������ ��ģ ��ü ���̰� 4096 ��ū�� ���� �ʾƾ� ���� �ǹ��մϴ�.

		GPT-4.0: �ִ� ��ū ������ �� 8000~16384 ��ū ���� ������ �� �� ��� ������ ���� �޶��� �� �ֽ��ϴ�.
		�̴� GPT-4�� ó���� �� �ִ� �Է°� ������ �� �ִ� ����� �� �հ� ���̿� ���� �����Դϴ�.
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

// Handle�� RichEdit ��Ʈ�ѿ��� �ؽ�Ʈ ��������
BOOL CDlgSummary::GetTextWithHandle(std::string& buf) {
	int lenText;

	buf = '\0';
	//lenText = GetTextSize(hRich);
	lenText = strEng.size();
	if (nParentRichPos < lenText) {
		//CharToWChar(buffer, (const char*)strEng.substr(nParentRichPos, lenText - nParentRichPos).c_str());
		//buf =StringToWString(strEng.substr(nParentRichPos, lenText - nParentRichPos).c_str());
		buf = strEng.substr(nParentRichPos, lenText - nParentRichPos).c_str();
		nParentRichPos = lenText;

		// �߰��� ���� ���� ���
		if (buf.size() == 0) return false;

		return true;
	}

	return false;
}

// RichEdit ��Ʈ�ѿ� �ؽ�Ʈ ����
void CDlgSummary::AddText(const std::wstring& text) {
	// �ؽ�Ʈ�� �������� Ŀ���� ��ġ��ŵ�ϴ�. -1, -1�� �ؽ�Ʈ�� ���� ��Ÿ���ϴ�.
	//SetRichEditTextColor2(hSumRichEdit, DlgSum->HexToCOLORREF(settings["ed_sumfont_color"]));
	SendMessage(hSumRichEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	//SendMessage(hSumRichEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)L"\r\n\r\n");
	SendMessage(hSumRichEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)text.c_str());
}

// RichEdit ��Ʈ�ѿ� �ؽ�Ʈ ������ ��������
int CDlgSummary::GetTextSize(HWND hRich) {
	// EM_GETLINECOUNT �޽����� ������ ���� ���� ����ϴ�.
	//int lineCount = SendMessage(hRich, EM_GETLINECOUNT, 0, 0);

	// �ؽ�Ʈ ������ �������� ( �����ڵ��� ��쿡 ����� 2���� �� ����)
	int ret = SendMessage(hRich, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0); // WPARAM�� LPARAM�� ������� �����Ƿ� 0���� �����մϴ�.
	return ret;
}

// RichEdit ��Ʈ���� ����
void CDlgSummary::Hide() {
	ShowWindow(hDlgSummary, SW_HIDE);
	bVisible = FALSE;
}

// RichEdit ��Ʈ���� ����
void CDlgSummary::Show() {
	ShowWindow(hDlgSummary, SW_SHOW);
	bVisible = TRUE;
}

// ��� ��ư Ŭ���� ��׶���� ���
BOOL CDlgSummary::BgSummary() {
	// Parent Richedit���� �ؽ�Ʈ ��������
	BOOL ret;
	char lang[10];
	std::string req;
	std::wstring strWOut;

	ret = GetTextWithHandle(req);
	if (ret == false) {
		//MessageBox(hwndDlg, L"�ؽ�Ʈ�� �������� ���߽��ϴ�.", L"����", MB_OK);
		return ret;
	}

	// req ����� 10k �ʰ��� �߶� ó���Ѵ�.
	int reqSize = req.size();
	std::string tmpReq;
	size_t pos = 0;

	while (1) {
		if (reqSize > 4096) {
			pos = req.find('\n', 4096);
			if (pos == std::string::npos) {
				pos = req.find(' ', 4096);
				if (pos == std::string::npos) {
					tmpReq = req.substr(0, 4096);
					req = req.substr(4096);
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

		// 20�� ���ϸ� skip
		if (tmpReq.size() < 20) break;

		// API ����
		// env���� ������� ��������
		if (settings["cb_summary_api"] == 0) {
			// openai API
			if (settings["cb_summary_lang"] == 0) strcpy_s(lang, "Korean");
			else strcpy_s(lang, "English");
			RequestOpenAIChat(tmpReq, strWOut, lang, settings["ed_summary_hint"], StringToWCHAR(settings.value("ed_summary_api_key", "")).c_str());
		}
		// child Richedit�� �ؽ�Ʈ �߰�
		rich_buf += L"\r\n\r\n"+strWOut;

		reqSize = req.size();
		if (reqSize <= 0) break;
	}

	addSummaryFlag = true;

	return true;
}

BOOL CDlgSummary::AddRichText() {
	if (addSummaryFlag == true) {
		AddText(rich_buf);
		GoBottom();
		rich_buf.clear();
		addSummaryFlag = false;
	}

	return true;
}

// �޽��� Ʈ���̽��� : Traceâ���� ����� ��� ���
BOOL CDlgSummary::Trace(const std::wstring& trace) {
	AddText(trace+L"\r\n");
	GoBottom();
	return true;
}

// �޽��� Ʈ���̽��� (���â)
void CDlgSummary::Alert(const WCHAR* msg) {
	MessageBox(hDlgSummary, msg, L"Alert", MB_OK);
}

// ���â�� �������� ��ġ�� ���� ���������� ���̰� �̵�
void CDlgSummary::GoBottom() {
	// Ŀ���� ������ ������ �̵���ŵ�ϴ�.
	SetFocus(hSumRichEdit);
	SendMessage(hSumRichEdit, EM_SETSEL, -1, 0);
}

// #c8c8c8 ������ COLORREF�� ��ȯ
COLORREF CDlgSummary::HexToCOLORREF(const std::string& hexCode) {
	// ���� �ڵ忡�� '#' ���ڸ� �����մϴ�.
	std::string hex = hexCode.substr(1);

	// 16���� ���� 10������ ��ȯ�մϴ�.
	int r = std::stoi(hex.substr(0, 2), nullptr, 16);
	int g = std::stoi(hex.substr(2, 2), nullptr, 16);
	int b = std::stoi(hex.substr(4, 2), nullptr, 16);

	// COLORREF ������ ��ȯ�մϴ�. RGB ��ũ�θ� ����մϴ�.
	return RGB(r, g, b);
}

// ��Ʈ ������ �����ϴ� �Լ�
void CDlgSummary::SetFont() {
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR;
	cf.yHeight = settings["cb_sumfont_size"] * 20; // ����Ʈ ������ ��Ʈ ũ�⸦ Ʈ��Ƽ������ ��ȯ
	//SendMessage(hRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

	//cf.dwMask = CFM_COLOR; // ���� ������ ��Ÿ���ϴ�.
	cf.crTextColor = HexToCOLORREF(settings["ed_sumfont_color"]); // ���ϴ� ������ RGB ������ �����մϴ�.

	// ��Ʈ ���� ������ �����մϴ�.
	SendMessage(hSumRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

//SetFocus(hRichEdit);
//SendMessage(hRichEdit, EM_SETSEL, end, -1);
//SetRichEditSelColor(hRichEdit, RGB(200, 200, 255));
//
//// ������ ����ϰ� �ʹٸ� ������ ���� �մϴ�.
//// �̴� Ŀ���� ������ ������ �̵���Ų �� ���� ������ ������ �մϴ�.
//SendMessage(hRichEdit, EM_SETSEL, -1, 0);