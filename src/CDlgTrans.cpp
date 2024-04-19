// ������ִ� richedit dialogue control class
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

// ������ �����ϴ� �Լ�
void CDlgTrans::Translate(std::string& strOut, std::string& srcLang, std::string& tgtLang) {
	// ������ �����.
	std::ofstream outFile("translate.txt");
	// ���Ͽ� ����.
	outFile<<srcLang << " -> " << tgtLang << std::endl;
	outFile <<strOut;
	// ������ �ݴ´�.
	outFile.close();
	isTrans = TRUE;
}

void CDlgTrans::CheckTransFile() {
	//if( isTrans!=TRUE ) return;

	// ������ �о���δ�.
	std::ifstream inFile("translate_out.txt");
	// ������ ������ ����
	if (!inFile.is_open()) {
		return;
	}

	SETTEXTEX st;
	st.flags = ST_DEFAULT; // �⺻ ���� ���
	st.codepage = 1200;    // UTF-16LE �ڵ� ������
	DWORD start = 0, end = 0;

	// ������ �о���δ�.
	std::string strTmp;
	while (std::getline(inFile, strTmp)) {
		// rich2�� �ؽ�Ʈ�� ����.
		if (hRich2 != NULL) {
			SendMessage(hRich2, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

			// Ŀ�� ��ġ(���� ������ ��)�� ����ϴ�.
			SendMessage(hRich2, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			SendMessage(hRich2, EM_REPLACESEL, (WPARAM)&st, (LPARAM)StringToWString(strTmp).c_str());
		}
	}
	// ������ �ݴ´�.
	inFile.close();

	// ������ �����Ѵ�.
	remove("translate_out.txt");
}

void GetWindowSize(HWND hWnd, int& width, int& height) {
	RECT rect;
	if (GetWindowRect(hWnd, &rect)) {
		width = rect.right - rect.left;   // �ʺ� ���
		height = rect.bottom - rect.top;  // ���� ���
	}
	else {
		// GetWindowRect ���� ��
		width = 0;
		height = 0;
	}
}

// ��� ��ư Ŭ���� ��׶���� ���
BOOL CDlgTrans::WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		CreateWindowEx(0, L"STATIC", APP_TITLE_VERSION, WS_CHILD | WS_VISIBLE | SS_CENTER,
			70, 20, 100, 20, hwndDlg, (HMENU)IDC_STATIC, GetModuleHandle(NULL), NULL);
		return (INT_PTR)TRUE;
	}
	case WM_PAINT:
		// �簢�� �׸���
	{
		// ������ : 34+10 = 43, 5+24+5 = 34
		// ������ : 1280 - 43 - 10 = 1280 - 53 =1227, 400 - 34 -5 = 400 - 39 = 361
		// �߰��� : 1227/2 = 613, [613-5=608]
		// �簢�� �׸��� : 43, 34, 600, 310, 648, 34, 600, 310
		// ������ â���� �׷��� �۾��� �����մϴ�.
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

		// ȸ�� �� ���� (ȸ������ �׵θ��� �׸� ����)
		HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
		// ������ ���� ����̽� ���ؽ�Ʈ�� ����
		SelectObject(hdc, hPen);

		// ���� �귯�� ���� (���θ� ����α� ����)
		HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
		// ������ �귯�ø� ����̽� ���ؽ�Ʈ�� ����
		SelectObject(hdc, hBrush);

		// ȸ�� �׵θ��� ���� ���θ� ���� �簢�� �׸���
		GetWindowSize(hwndDlg, window_width, window_height);
		int width = (window_width - 64) / 2; // �������� �ʺ񿡼� ������ �� ��
		int height = window_height - 42; // �������� ���̿��� ������ �� ��
		Rectangle(hdc, 43, 33, 35+width, window_height - 50);
		Rectangle(hdc, 38+width, 33, window_width-29, window_height - 50);

		//MoveWindow(hRich1, 44, 34, width, height, TRUE);
		//MoveWindow(hRich2, 54 + width, 34, width, height, TRUE);

		// ����� ���� GDI ������Ʈ ����
		DeleteObject(hPen);
		DeleteObject(hBrush); // �����δ� NULL_BRUSH�� �ý��� ������Ʈ�̹Ƿ� �������� �ʾƵ� �˴ϴ�.

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam); // �޴� ����, ��ư Ŭ�� ���� ��Ʈ�� �ĺ���
		int wmEvent = HIWORD(wParam); // �˸� �ڵ�

		switch (wmId) {
		case IDCANCEL:
		case IDOK:
			return (INT_PTR)TRUE;
			break;
		// ���� ��ư Ŭ���ÿ� ������ ����
		case IDC_BTN_TRANSLATE:
			// �Ѱ��� ���� ������ ����
			std::string strTmp, srcLang, tgtLang;
			GetText(strTmp, srcLang, tgtLang);
			// ������ �����Ѵ�.
			Translate(strTmp, srcLang, tgtLang);
			break;
		}
	}
	break;
	case WM_SIZE:
	{
		int width, height;
		window_width = LOWORD(lParam); // ���ο� �������� �ʺ�
		window_height = HIWORD(lParam); // ���ο� �������� ����
		width = (window_width - 64)/2; // �������� �ʺ񿡼� ������ �� ��
		height = window_height - 42; // �������� ���̿��� ������ �� ��

		// �ؽ�Ʈ�ڽ��� ũ��� ��ġ�� �������� ũ�⿡ ���߾� ����
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
		// �ʿ��� ���� �۾� ����
		//EndDialog(hwndDlg, 0);
		//DestroyWindow(hwndDlg);
		return 0;
	}
	return (INT_PTR)FALSE;
}

// Ŭ���� ����
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
	TCHAR buffer[256];    // ���ڿ��� ������ ����

	LoadString(hInstance, IDS_TRANSLATE, buffer, 256);

	// ����	��ư
	CreateWindow(
		L"BUTTON", buffer,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		43, 5, 200, 24,
		hWnd, (HMENU)IDC_BTN_TRANSLATE, GetModuleHandle(nullptr), nullptr
	);
	// ���� ��� ���� �޺��ڽ�
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST| WS_VSCROLL,
		253, 5, 150, 24,
		hWnd, (HMENU)IDC_CB_SRCLANG, GetModuleHandle(nullptr), nullptr
	);
	// '->'�� ��Ÿ���� static control �߰�
	CreateWindow(
		L"STATIC", L"->",
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		403, 5, 20, 24,
		hWnd, (HMENU)IDC_STATIC_ARROW, GetModuleHandle(nullptr), nullptr
	);
	
	// ���� ��� ���� �޺��ڽ�
	CreateWindow(
		L"COMBOBOX", L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST| WS_VSCROLL,
		423, 5, 150, 24,
		hWnd, (HMENU)IDC_CB_TGTLANG, GetModuleHandle(nullptr), nullptr
	);

	// ���� ���� ������ ���� �迭�� �ʱ�ȭ
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

	// ���� �ؽ�Ʈ �ڽ�
	HWND hListBox = GetDlgItem(hWnd, IDC_CB_SRCLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
		}
		SendMessage(hListBox, CB_SETCURSEL, (WPARAM)1, 0);
	}
	// ��� �ؽ�Ʈ �ڽ�
	hListBox = GetDlgItem(hWnd, IDC_CB_TGTLANG);
	if (hListBox != NULL) {
		for (int i = 0; i < lenLang; ++i) {
			index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
			SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
		}
		SendMessage(hListBox, CB_SETCURSEL, (WPARAM)0, 0);
	}

	// �� : 1280-43 = 1237, 1237/2 = 618
	// ���� : 400-34-5 = 361
	// richedit ��Ʈ�� �߰� �¿� 2��
	hRich1 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE, // | ES_READONLY,
		44, 34, 590, 310, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

	hRich2 = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE, // | ES_READONLY,
		648, 34, 600, 310, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR; // ��Ʈ ũ��� ���� ������ ��Ÿ���ϴ�.
	cf.yHeight = 12 * 20; // ����Ʈ ������ ��Ʈ ũ�⸦ Ʈ��Ƽ������ ��ȯ
	cf.crTextColor = RGB(200, 200, 255);; // ���ϴ� ������ RGB ������ �����մϴ�.

	// ��Ʈ ���� ������ �����մϴ�.
	SendMessage(hRich1, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SendMessage(hRich2, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

	// ���� ����
	SendMessage(hRich1, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));
	SendMessage(hRich2, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));
}

void CDlgTrans::ShowDialogControls() {
	ShowWindow(hRich1, SW_SHOW);
	ShowWindow(hRich2, SW_SHOW);
	// ��ư ���̱�
	ShowWindow(GetDlgItem(hWnd, IDC_BTN_TRANSLATE), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_SRCLANG), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_TGTLANG), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARROW), SW_SHOW);
}

void CDlgTrans::HideDialogControls() {
	ShowWindow(hRich1, SW_HIDE);
	ShowWindow(hRich2, SW_HIDE);
	// ��ư �����
	ShowWindow(GetDlgItem(hWnd, IDC_BTN_TRANSLATE), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_SRCLANG), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_CB_TGTLANG), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARROW), SW_HIDE);
}

// RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
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

// RichEdit ��Ʈ���� ����
void CDlgTrans::Hide() {
	//ShowWindow(hDlg, SW_HIDE);
	HideDialogControls();
	bVisible = FALSE;
}

// RichEdit ��Ʈ���� ����
void CDlgTrans::Show() {
	//ShowWindow(hDlg, SW_SHOW);
	ShowDialogControls();
	bVisible = TRUE;
}

// rich1�� �ؽ�Ʈ�� ������
void CDlgTrans::GetText(std::string &strOut, std::string &srcLang, std::string &tgtLang) {
	LPARAM itemData;
	// �ؽ�Ʈ ���̸� �����´�.
	int nLen = GetWindowTextLength(hRich1);
	// �ؽ�Ʈ ���̸�ŭ ���۸� �Ҵ��Ѵ�.
	WCHAR* pBuf = new WCHAR[nLen + 1];
	// ���ۿ� �ؽ�Ʈ�� �����Ѵ�.
	GetWindowText(hRich1, pBuf, nLen + 1);
	// ������ ������ std::string���� ��ȯ�Ѵ�.
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

// �޽��� Ʈ���̽��� (���â)
void CDlgTrans::Alert(const WCHAR* msg) {
	MessageBox(hWnd, msg, L"Alert", MB_OK);
}