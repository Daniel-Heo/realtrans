#include "CSettings.h"
#include <fstream>
#include <iostream>
#include <stdexcept> // std::invalid_argument, std::out_of_range 예외를 처리하기 위해 포함
#include "Resource.h"
#include "json.hpp" // 또는 경로를 지정해야 할 경우: #include "External/json.hpp"
#include "StrConvert.h"
#include "CDlgSummary.h"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

// 환경 설정을 위한 JSON 객체 생성
extern json settings;
extern BOOL isRefreshEnv;
extern time_t nSummaryTime;
extern CDlgSummary* DlgSum;

// JSON 객체를 문자열로 출력하는 함수
void PrintJson()
{
	// JSON 객체 생성 및 사용 예시
	json j;
	j["name"] = "John";
	j["age"] = 30;
	j["is_student"] = false;
	j["skills"] = { "C++", "Python", "JavaScript" };

	// JSON 객체를 문자열로 출력
	//std::cout << j.dump(4) << std::endl;
}

// Python 프로그램에 전달할 명령을 childcmd.temp 파일로 저장하는 함수 : Python 프로그램은 childcmd.temp 파일을 읽어서 명령을 수행
void MakeChildCmd(const char* cmd)
{
	std::ofstream file("childcmd.temp");
	file << cmd;
}

// 현재 설정을 파일로 저장
void SaveSettings(HWND hwnd, const std::string& filePath) {
	BOOL old_pctrans = settings["ck_pctrans"];

	// Checkbox 상태 읽기 (예시: 체크박스의 ID가 IDC_CHECKBOX1이라고 가정)
	bool checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_VSUB) == BST_CHECKED);
	settings["ck_orgin_subtext"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_PCTRANS) == BST_CHECKED);
	settings["ck_pctrans"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_APITRANS) == BST_CHECKED);
	settings["ck_apitrans"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_SUMMARY) == BST_CHECKED);
	settings["ck_summary"] = checkBoxState;

	if (settings["ck_pctrans"] != old_pctrans)
	{
		if(settings["ck_pctrans"] )
			MakeChildCmd("ko");
		else
			MakeChildCmd("xx");
	}

	// Combobox 선택된 아이템 읽기 (예시: 콤보박스의 ID가 IDC_COMBOBOX1이라고 가정)
	WCHAR comboBoxItem[256];
	HWND hCbBox = GetDlgItem(hwnd, IDC_COMBO_VOICE_LANG);
	settings["cb_voice_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_PCTRANS_LANG);
	settings["cb_pctrans_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_SUMMARY);
	settings["cb_summary_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	/*GetDlgItemText(hwnd, IDC_EDIT_TRANS_API_SEC, comboBoxItem, 256);
	settings["cb_apitrans_sec"] = wstringToInt(comboBoxItem);*/

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_APITRANS_LANG);
	settings["cb_apitrans_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_TRANS_API);
	settings["cb_trans_api"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	GetDlgItemText(hwnd, IDC_EDIT_SUMMARY_MIN, comboBoxItem, 256);
	settings["cb_summary_min"] = wstringToInt(comboBoxItem);

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_SUM_API);
	settings["cb_summary_api"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	GetDlgItemText(hwnd, IDC_EDIT_FONT_COLOR, comboBoxItem, 256);
	settings["ed_font_color"]= WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_OLDFONT_COLOR, comboBoxItem, 256);
	settings["ed_oldfont_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_SUMFONT_COLOR, comboBoxItem, 256);
	settings["ed_sumfont_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_FONT_SIZE, comboBoxItem, 256);
	settings["cb_font_size"] = wstringToInt(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_OLDFONT_SIZE, comboBoxItem, 256);
	settings["cb_oldfont_size"] = wstringToInt(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_SUMFONT_SIZE, comboBoxItem, 256);
	settings["cb_sumfont_size"] = wstringToInt(comboBoxItem);

	// API Key
	GetDlgItemText(hwnd, IDC_EDIT_TRANS_API_KEY, comboBoxItem, 256);
	settings["ed_trans_api_key"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_SUMMARY_API_KEY, comboBoxItem, 256);
	settings["ed_summary_api_key"] = WCHARToString(comboBoxItem);

	// 요약 힌트
	WCHAR lpString[512];
	GetDlgItemText(hwnd, IDC_EDIT_SUMMARY_HINT, lpString, 512);
	settings["ed_summary_hint"] = WCHARToString(lpString);

	// 요약 갱신시간 초기화
	nSummaryTime = 0;

	// 폰트 초기화
	DlgSum->SetFont();

	// JSON 파일로 저장
	std::ofstream file(filePath);
	file << settings.dump(4); // 예쁘게 출력하기 위해 4를 들여쓰기로 사용
}

// JSON 객체에 기본값 설정 : 초기화 시에도 사용
void defaultJson() {
	settings["ck_orgin_subtext"] = false; // 원문 자막 표시
	settings["ck_pctrans"] = true; // 자동 번역 PC
	settings["ck_apitrans"] = false; // 자동 번역 API
	settings["ck_summary"] = false; // 요약 API 사용

	settings["cb_voice_lang"] = 0; // 음성 언어
	settings["cb_pctrans_lang"] = 0; // 번역 결과 언어 ( PC )
	settings["cb_summary_lang"] = 0; // 요약 언어
	settings["cb_apitrans_sec"] = 1000; // API 번역 주기 : Sec
	settings["cb_apitrans_lang"] = 0; // 번역 결과 언어 ( API )
	settings["cb_trans_api"] = 0; // 번역에 사용할 API
	settings["cb_summary_min"] = 1; // 요약 주기 : Minute
	settings["cb_summary_api"] = 0; // 요약에 사용할 API

	// 폰트 컬러
	settings["ed_font_color"] = "#C8C8FF";
	settings["ed_oldfont_color"] = "#8C8C8C";
	settings["cb_font_size"] = 20;
	settings["cb_oldfont_size"] = 20;
	// 요약창 폰트 컬러
	settings["ed_sumfont_color"] = "#C8C8FF";
	settings["cb_sumfont_size"] = 13;

	settings["ed_trans_api_key"] = ""; // 번역에 사용할 API Key
	settings["ed_summary_api_key"] = ""; // 요약에 사용할 API Key
	settings["ed_summary_hint"] = "The following text is a conversation related to securities and economics. Please summarize the main topics, key data points, and conclusions in around 300 words. The summary should be arrange them in number list form to keep them short and concise."; // 요약 힌트
}

// 설정 화면 갱신
void RefreshSettings(HWND hwnd, BOOL isStart)
{
	WCHAR tmp[10];

	// 원문 자막 표시
	CheckDlgButton(hwnd, IDC_CHECK_VSUB, settings["ck_orgin_subtext"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 자동 번역 PC
	CheckDlgButton(hwnd, IDC_CHECK_PCTRANS, settings["ck_pctrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 자동 번역 API
	CheckDlgButton(hwnd, IDC_CHECK_APITRANS, settings["ck_apitrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 요약 API
	CheckDlgButton(hwnd, IDC_CHECK_SUMMARY, settings["ck_summary"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);

	// 번역 입력 언어
	HWND hListBox = GetDlgItem(hwnd, IDC_COMBO_VOICE_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_voice_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 번역 결과 언어 ( PC )
	hListBox = GetDlgItem(hwnd, IDC_COMBO_PCTRANS_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_pctrans_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 요약 언어
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUMMARY); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 번역 결과 언어 ( API )
	hListBox = GetDlgItem(hwnd, IDC_COMBO_APITRANS_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_apitrans_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 번역에 사용할 API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_TRANS_API); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"DeepL");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_trans_api"].get<int>(), 0); // 두번째 1,0
	}

	// 요약에 사용할 API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUM_API); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			//SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Google");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"OpenAI");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_api"].get<int>(), 0); // 두번째 1,0
	}

	// 요약 주기 : Minute
	wsprintf(tmp, L"%d", settings["cb_summary_min"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_MIN, tmp);

	// 폰트 사이즈 
	wsprintf(tmp, L"%d", settings["cb_font_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_FONT_SIZE, tmp);

	// 지나간 폰트 사이즈 
	wsprintf(tmp, L"%d", settings["cb_oldfont_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_SIZE, tmp);

	// 요약 폰트 사이즈 
	wsprintf(tmp, L"%d", settings["cb_sumfont_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_SIZE, tmp);

	// 폰트 컬러
	SetDlgItemText(hwnd, IDC_EDIT_FONT_COLOR, StringToWCHAR(settings.value("ed_font_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_COLOR, StringToWCHAR(settings.value("ed_oldfont_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_COLOR, StringToWCHAR(settings.value("ed_sumfont_color", "")).c_str());

	// API Key
	SetDlgItemText(hwnd, IDC_EDIT_TRANS_API_KEY, StringToWCHAR(settings.value("ed_trans_api_key", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_API_KEY, StringToWCHAR(settings.value("ed_summary_api_key", "")).c_str());

	// 요약 힌트
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_HINT, StringToWCHAR(settings.value("ed_summary_hint", "")).c_str());
}

// 초기화
void InitSettings(HWND hwnd) {
	// settings에 Default 값 셋팅
	defaultJson();
	RefreshSettings(hwnd, false);
}

// 설정 파일 읽기
void ReadSettings(const std::string& filePath) {
	// 파일에서 JSON 데이터 읽기
	std::ifstream file(filePath);
	if (file.is_open()) {
		file >> settings;
		//std::cout << "입력 받은 문자열 :: " << file.dump() << std::endl;
	}
	else {
		defaultJson();
	}
}

// 설정 파일을 읽고 현재 설정에	적용
void LoadSettings(HWND hwnd, const std::string& filePath)
{
	ReadSettings(filePath);

	RefreshSettings(hwnd, true);
}

// Setting Dialog 메시지 처리
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	//if (GetCurrentThreadId() != g_MainUIThreadID) return (INT_PTR)FALSE;

	switch (uMsg) {
	case WM_INITDIALOG:
	{
		// 다이얼로그 초기화 코드 시작
		LoadSettings(hwndDlg, "config.json");

		return (INT_PTR)TRUE;
	}
	case WM_PAINT:
	{

	}
	break;
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);

			switch (wmId)
			{
			case IDCANCEL: // 취소
				EndDialog(hwndDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			case IDSUPPLY: // 저장 및 종료
				SaveSettings(hwndDlg, "config.json");
				isRefreshEnv = true;
				EndDialog(hwndDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			case ID_INIT: // 초기화
				if (wmEvent == BN_CLICKED)
					InitSettings(hwndDlg);
				//return (INT_PTR)TRUE;
				break;

			case IDC_CHECK_PCTRANS:
				if (wmEvent == BN_CLICKED) {
					// 체크박스 상태 읽기
					bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PCTRANS) == BST_CHECKED);
					if (checked) {
						// 첫 번째 체크박스가 체크되어 있으면, 두 번째 체크박스를 체크 해제합니다. : 중복사용을 위해 막음
						//CheckDlgButton(hwndDlg, IDC_CHECK_APITRANS, BST_UNCHECKED);
					}
				}
				break;
			case IDC_CHECK_APITRANS:
				if (wmEvent == BN_CLICKED) {
					// 체크박스 상태 읽기
					bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_APITRANS) == BST_CHECKED);
					if (checked) {
						// 첫 번째 체크박스가 체크되어 있으면, 두 번째 체크박스를 체크 해제합니다. : 중복사용을 위해 막음
						//CheckDlgButton(hwndDlg, IDC_CHECK_PCTRANS, BST_UNCHECKED);
					}
				}
				break;

			}
		}
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}