// 요약해주는 richedit dialogue control class
#include "CDlgInfo.h"
#include <commctrl.h> // Common Controls 헤더
#include "StrConvert.h"
#include "version.h"
#pragma comment(lib, "comctl32.lib") //adds link to control control DLL
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern CDlgInfo* DlgInfo;

// 요약 버튼 클릭시 백그라운드로 요약
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
			int wmId = LOWORD(wParam); // 메뉴 선택, 버튼 클릭 등의 컨트롤 식별자
			int wmEvent = HIWORD(wParam); // 알림 코드

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
		// 필요한 정리 작업 수행
		EndDialog(hwndDlg, 0);
		DestroyWindow(hwndDlg);
		return 0;
	}
	return (INT_PTR)FALSE;
}

// 클래스 선언
CDlgInfo::CDlgInfo(HINSTANCE hInst) {
	hInstance = hInst;
	bVisible = FALSE;
	bExistDlg = FALSE;
}

// RichEdit 컨트롤을 생성하고 초기화하는 함수
void CDlgInfo::Create(HWND hWndP) {
	// sysLink 초기화 : Common Controls 초기화 ( About Dialog 사용을 위해 필요 )
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS; // SysLink 컨트롤 사용을 위해 필요
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

// RichEdit 컨트롤을 숨김
void CDlgInfo::Hide() {
	ShowWindow(hDlgInfo, SW_HIDE);
	bVisible = FALSE;
}

// RichEdit 컨트롤을 숨김
void CDlgInfo::Show() {
	ShowWindow(hDlgInfo, SW_SHOW);
	bVisible = TRUE;
}

// 메시지 트레이스용 (경고창)
void CDlgInfo::Alert(const WCHAR* msg) {
	MessageBox(hDlgInfo, msg, L"Alert", MB_OK);
}