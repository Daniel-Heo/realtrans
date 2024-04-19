// ������ִ� richedit dialogue control class
#include "CDlgInfo.h"
#include <commctrl.h> // Common Controls ���
#include "StrConvert.h"
#include "version.h"
#pragma comment(lib, "comctl32.lib") //adds link to control control DLL
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern CDlgInfo* DlgInfo;

// ��� ��ư Ŭ���� ��׶���� ���
INT_PTR CALLBACK InfoProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		CreateWindowEx(0, L"STATIC", APP_TITLE_VERSION, WS_CHILD | WS_VISIBLE | SS_CENTER,
			70, 20, 100, 20, hwndDlg, (HMENU)IDC_STATIC, GetModuleHandle(NULL), NULL);
		return (INT_PTR)TRUE;
	}
	case WM_PAINT:
		break;
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam); // �޴� ����, ��ư Ŭ�� ���� ��Ʈ�� �ĺ���
			int wmEvent = HIWORD(wParam); // �˸� �ڵ�

			switch (wmId) {
				case IDCANCEL:
				case IDOK:
				{
					EndDialog(hwndDlg, LOWORD(wParam));
					DestroyWindow(hwndDlg);
					return (INT_PTR)TRUE;
				}
				break;
			}
		}
		break;
	case WM_NOTIFY:
		//detects change in link control
		switch (((LPNMHDR)lParam)->code)
		{
			case NM_CLICK:
			case NM_RETURN:
				{
					PNMLINK pNMLink = (PNMLINK)lParam;
					char url[2048];
					size_t len;
					//DlgInfo->Alert((const WCHAR*)pNMLink->item.szUrl);
					wcstombs_s(&len, url, (size_t)2048,	pNMLink->item.szUrl, (size_t)2048 - 1);
					ShellExecuteA(0, "open", "chrome.exe", (LPCSTR)url, NULL, SW_SHOWNORMAL);
					break;
				}

		}
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		DestroyWindow(hwndDlg);
		return (INT_PTR)TRUE;

	case WM_DESTROY:
		// �ʿ��� ���� �۾� ����
		EndDialog(hwndDlg, 0);
		DestroyWindow(hwndDlg);
		return 0;
	}
	return (INT_PTR)FALSE;
}

// Ŭ���� ����
CDlgInfo::CDlgInfo(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
}

// RichEdit ��Ʈ���� �����ϰ� �ʱ�ȭ�ϴ� �Լ�
void CDlgInfo::Create(HWND hWndP) {
	// sysLink �ʱ�ȭ : Common Controls �ʱ�ȭ ( About Dialog ����� ���� �ʿ� )
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS; // SysLink ��Ʈ�� ����� ���� �ʿ�
	InitCommonControlsEx(&icex);

	if (bExistDlg == FALSE) {
		hWndParent = hWndP;
		hDlgInfo = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWndParent, InfoProc);
		bExistDlg = TRUE;
	}

	if (bVisible == FALSE) {
		Show();
	}
}

// RichEdit ��Ʈ���� ����
void CDlgInfo::Hide() {
	ShowWindow(hDlgInfo, SW_HIDE);
	bVisible = FALSE;
}

// RichEdit ��Ʈ���� ����
void CDlgInfo::Show() {
	ShowWindow(hDlgInfo, SW_SHOW);
	bVisible = TRUE;
}

// �޽��� Ʈ���̽��� (���â)
void CDlgInfo::Alert(const WCHAR* msg) {
	MessageBox(hDlgInfo, msg, L"Alert", MB_OK);
}