#include "CSettings.h"
#include <fstream>
#include <iostream>
#include <stdexcept> // std::invalid_argument, std::out_of_range 예외를 처리하기 위해 포함
#include "Resource.h"
#include "json.hpp" 
#include "StrConvert.h"
#include "CDlgSummary.h"
#include "language.h"
#include "BgJob.h"
#include "RealTrans.h"

// 기본 모델 크기
#define DEFULAT_MODEL_SIZE "small"
#define DEFULAT_PROC "nvidia float16"
#define DEFULAT_INPUT_DEV "speaker"
#define DEFULAT_TRANSPARENT 95
#define DEFULAT_BG_COLOR "#000000"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

// 환경 설정을 위한 JSON 객체 생성
extern json settings;

// 싱글톤 인스턴스 초기화
CSettings* CSettings::s_instance = nullptr;

// 다이얼로그 참조를 위한 외부 변수 처리
//extern CDlgSummary* DlgSum;
//extern std::string addText;

// 번역 결과 언어
WCHAR langArr[32][2][40] = {
	{L"Korean", L"KO"}, {L"American English", L"EN-US"}, {L"British English", L"EN-GB"}, {L"Arabic", L"AR"}, {L"Bulgarian", L"BG"},
	{L"Czech", L"CS"}, {L"Danish", L"DA"}, {L"German", L"DE"}, {L"Greek", L"EL"}, {L"Spanish", L"ES"}, {L"Estonian", L"ET"},
	{L"Finnish", L"FI"}, {L"French", L"FR"}, {L"Hungarian", L"HU"}, {L"Indonesian", L"ID"}, {L"Italian", L"IT"}, {L"Japanese", L"JA"},
	{L"Lithuanian", L"LT"}, {L"Latvian", L"LV"}, {L"Norwegian", L"NB"}, {L"Dutch", L"NL"}, {L"Polish", L"PL"}, {L"Brazilian Portuguese", L"PT-BR"},
	{L"Portuguese", L"PT-PT"}, {L"Romanian", L"RO"}, {L"Russian", L"RU"}, {L"Slovak", L"SK"}, {L"Slovenian", L"SL"}, {L"Swedish", L"SV"},
	{L"Turkish", L"TR"}, {L"Ukrainian", L"UK"}, {L"Chinese", L"ZH"}
};

// 환경설정 언어,  요약 언어
LPCWSTR langEnv[] = {
	L"Korean",
	L"English",
	L"German",
	L"Spanish",
	L"French",
	L"Italian",
	L"Japanese",
	L"Portuguese",
	L"Russian",
	L"Chinese",
	L"Arabic",
	L"Hindi"
};

// 생성자
CSettings::CSettings() : m_hDlg(NULL) {
	// 기본값 초기화
	DefaultJson();
	m_hInst = GetModuleHandle(NULL);
}

// 소멸자
CSettings::~CSettings() {
	// 필요한 정리 작업
	if (m_hDlg != NULL) {
		DestroyWindow(m_hDlg);
		m_hDlg = NULL;
	}
}

// 싱글톤 인스턴스 접근자
CSettings* CSettings::GetInstance() {
	if (s_instance == nullptr) {
		s_instance = new CSettings();
	}
	return s_instance;
}

// 인스턴스 소멸
void CSettings::DestroyInstance() {
	if (s_instance != nullptr) {
		delete s_instance;
		s_instance = nullptr;
	}
}

// 다이얼로그 생성 메서드
BOOL CSettings::Create(HWND hWndParent) {
	// 이미 생성된 다이얼로그가 있으면 포커스를 주고 종료
	if (m_hDlg != NULL) {
		SetFocus(m_hDlg);
		return TRUE;
	}

	// 다이얼로그 생성시 this 포인터를 lParam으로 전달
	m_hDlg = CreateDialogParam(
		m_hInst,
		MAKEINTRESOURCE(IDD_DIALOG_SETUP),
		hWndParent,
		StaticDialogProc,
		(LPARAM)this
	);

	if (m_hDlg == NULL) {
		DWORD error = GetLastError();
		char buffer[256];
		sprintf_s(buffer, "CreateDialogParam failed with error %d", error);
		MessageBoxA(NULL, buffer, "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// 다이얼로그 표시
	ShowWindow(m_hDlg, SW_SHOW);
	return TRUE;
}

// Python 프로그램에 전달할 명령을 pymsg.json 파일로 저장하는 함수 : Python 프로그램은 pymsg.json 파일을 읽어서 명령을 수행
void CSettings::MakeChildCmd(const std::string& src_lang, const std::string& tgt_lang)
{
	std::string txt = "{ \"src_lang\": \"" + src_lang + "\", \"tgt_lang\": \"" + tgt_lang + "\" }";
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + "pymsg.json";
	std::ofstream file(strFileName);
	if (!file.is_open()) {
		std::cerr << "Error: Unable to open file for writing: " << strFileName << std::endl;
		return; // 파일을 열지 못했을 때 오류 코드 반환
	}
	file << txt;
	file.close(); // 파일을 닫음
}

// 설정파일 저장
void CSettings::SaveSimpleSettings() {
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + "config.json";
	std::ofstream file(strFileName);
	if (file.is_open()) {
		file << settings.dump(4);
		file.close();
	}
}

// 현재 설정을 파일로 저장
void CSettings::SaveSettings(HWND hwnd, const std::string& filePath) {
	BOOL old_pctrans = settings["ck_pctrans"];
	LRESULT nSel = 0;
	LPARAM itemData;
	HWND hCbBox;
	WCHAR comboBoxItem[256];

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_UI_LANG);
	settings["ui_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	// 모델 크기 설정
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_MODEL_SIZE);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	SendMessage(hCbBox, CB_GETLBTEXT, nSel, (LPARAM)comboBoxItem);
	settings["model_size"] = WCHARToString((WCHAR*)comboBoxItem);

	// 음성 입력장치 설정
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_INPUT_DEV);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	SendMessage(hCbBox, CB_GETLBTEXT, nSel, (LPARAM)comboBoxItem);
	settings["input_dev"] = WCHARToString((WCHAR*)comboBoxItem);

	// 처리 방식 설정
	hCbBox = GetDlgItem(hwnd, IDC_COMBO_PROC);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	SendMessage(hCbBox, CB_GETLBTEXT, nSel, (LPARAM)comboBoxItem);
	settings["proc"] = WCHARToString((WCHAR*)comboBoxItem);

	// 언어 설정
	switch (settings["ui_lang"].get<int>())
	{
	case 0: 	SetUserLocale(L"en-US");
		break;
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
	}

	// Checkbox 상태 읽기 (예시: 체크박스의 ID가 IDC_CHECKBOX1이라고 가정)
	bool checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_VSUB) == BST_CHECKED);
	settings["ck_orgin_subtext"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_PCTRANS) == BST_CHECKED);
	settings["ck_pctrans"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_APITRANS) == BST_CHECKED);
	settings["ck_apitrans"] = checkBoxState;
	checkBoxState = (IsDlgButtonChecked(hwnd, IDC_CHECK_SUMMARY) == BST_CHECKED);
	settings["ck_summary"] = checkBoxState;

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_VOICE_LANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		settings["cb_voice_lang"] = WCHARToString((WCHAR*)itemData);
	}

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_PCTRANS_LANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		settings["cb_pctrans_lang"] = WCHARToString((WCHAR*)itemData);
	}

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_APITRANS_LANG);
	nSel = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);
	itemData = SendMessage(hCbBox, CB_GETITEMDATA, nSel, 0);
	if (itemData != CB_ERR) {
		settings["cb_apitrans_lang"] = WCHARToString((WCHAR*)itemData);
	}
	// json 파일로 저장
	std::string tgt_lang;
	if (settings["ck_pctrans"] == true) tgt_lang = settings["cb_pctrans_lang"];
	else tgt_lang = "xx";
	MakeChildCmd(settings["cb_voice_lang"], tgt_lang);

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_SUMMARY);
	settings["cb_summary_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_TRANS_API);
	settings["cb_trans_api"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	GetDlgItemText(hwnd, IDC_EDIT_SUMMARY_MIN, comboBoxItem, 256);
	settings["cb_summary_min"] = wstringToInt(comboBoxItem);

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_SUM_API);
	settings["cb_summary_api"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

	GetDlgItemText(hwnd, IDC_EDIT_FONT_COLOR, comboBoxItem, 256);
	settings["ed_font_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_OLDFONT_COLOR, comboBoxItem, 256);
	settings["ed_oldfont_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_BG_COLOR, comboBoxItem, 256);
	settings["bg_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_SUMFONT_COLOR, comboBoxItem, 256);
	settings["ed_sumfont_color"] = WCHARToString(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_FONT_SIZE, comboBoxItem, 256);
	settings["cb_font_size"] = wstringToInt(comboBoxItem);

	//GetDlgItemText(hwnd, IDC_EDIT_OLDFONT_SIZE, comboBoxItem, 256);
	//settings["cb_oldfont_size"] = wstringToInt(comboBoxItem);

	GetDlgItemText(hwnd, IDC_EDIT_SUMFONT_SIZE, comboBoxItem, 256);
	settings["cb_sumfont_size"] = wstringToInt(comboBoxItem);

	GetDlgItemText(hwnd, IDC_COMBO_TRANSPARENT, comboBoxItem, 256);
	settings["transparent"] = wstringToInt(comboBoxItem);

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
	BgJob::GetInstance()->ResetSummaryTime();

	// 폰트 초기화
	RealTrans::GetInstance()->SetSummaryFont();

	// JSON 파일로 저장
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + filePath;
	std::ofstream file(strFileName.c_str());
	if (file.is_open() == false) {
		std::cerr << "Error: Unable to open file for writing: " << strFileName << std::endl;
		return;
	}
	file << settings.dump(4); // 예쁘게 출력하기 위해 4를 들여쓰기로 사용
	file.close(); // 파일을 닫음
}

// JSON 객체에 기본값 설정 : 초기화 시에도 사용
void CSettings::DefaultJson() {
	// 언어 모델 크기 설정
	settings["ui_lang"] = 0; // 환경설정 언어 설정
	settings["model_size"] = DEFULAT_MODEL_SIZE; // 모델 사이즈

	// 입력장치 및 처리방식 설정
	settings["input_dev"] = "speaker";
	settings["proc"] = "nvidia float16"; // 처리장치 설정

	settings["txt_trans_src"] = "eng_Latn"; // 텍스트 번역 소스 언어
	settings["txt_trans_tgt"] = "kor_Hang"; // 텍스트 번역 대상 언어	

	settings["ck_orgin_subtext"] = false; // 원문 자막 표시
	settings["ck_pctrans"] = true; // 자동 번역 PC
	settings["ck_apitrans"] = false; // 자동 번역 API
	settings["ck_summary"] = false; // 요약 API 사용

	settings["cb_voice_lang"] = "eng_Latn"; // 음성 언어
	settings["cb_pctrans_lang"] = "kor_Hang"; // 번역 결과 언어 ( PC )
	settings["cb_summary_lang"] = 0; // 요약 언어
	settings["cb_apitrans_lang"] = "KO"; // 번역 결과 언어 ( API )
	settings["cb_trans_api"] = 0; // 번역에 사용할 API
	settings["cb_summary_min"] = 1; // 요약 주기 : Minute
	settings["cb_summary_api"] = 0; // 요약에 사용할 API

	// 폰트 컬러
	settings["ed_font_color"] = "#C8C8FF";
	settings["ed_oldfont_color"] = "#8C8C8C";
	settings["cb_font_size"] = 18;
	settings["cb_oldfont_size"] = 18;
	// 요약창 폰트 컬러
	settings["ed_sumfont_color"] = "#C8C8FF";
	settings["cb_sumfont_size"] = 13;
	// 배경 컬러
	settings["bg_color"] = DEFULAT_BG_COLOR;
	// 투명도
	settings["transparent"] = DEFULAT_TRANSPARENT;

	settings["ed_trans_api_key"] = ""; // 번역에 사용할 API Key
	settings["ed_summary_api_key"] = ""; // 요약에 사용할 API Key
	settings["ed_summary_hint"] = "The following text is a conversation related to securities and economics. Please summarize the main topics, key data points, and conclusions in around 300 words. The summary should be arrange them in number list form to keep them short and concise."; // 요약 힌트
}

// 설정 화면 갱신
void CSettings::RefreshSettings(HWND hwnd, BOOL isStart)
{
	WCHAR tmp[256];
	int index;
	int itemCount;
	HWND hListBox;

	// 모델 사이즈
	hListBox = GetDlgItem(hwnd, IDC_COMBO_MODEL_SIZE); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"large");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"medium");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"small");
		}

		if (settings.find("model_size") == settings.end()) settings["model_size"] = DEFULAT_MODEL_SIZE;
		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			SendMessage(hListBox, CB_GETLBTEXT, (WPARAM)i, (LPARAM)tmp);
			if (CompareStringWString(settings.value("model_size", ""), (WCHAR*)tmp) == true) {
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 환경 설정 언어
	hListBox = GetDlgItem(hwnd, IDC_COMBO_UI_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"None");

			// languages 배열을 순회하며 각 언어를 hListBox에 추가합니다.
			int lenLang = sizeof(langEnv) / sizeof(langEnv[0]);
			for (int i = 0; i < lenLang; ++i) {
				SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)langEnv[i]);
			}
		}

		SendMessage(hListBox, CB_SETCURSEL, settings["ui_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 처리방식
	hListBox = GetDlgItem(hwnd, IDC_COMBO_PROC); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"nvidia float16");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"nvidia float32");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"amd float16");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"amd float32");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"CPU");
		}

		if (settings.find("proc") == settings.end()) settings["proc"] = DEFULAT_PROC;
		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			SendMessage(hListBox, CB_GETLBTEXT, (WPARAM)i, (LPARAM)tmp);
			if (CompareStringWString(settings.value("proc", ""), (WCHAR*)tmp) == true) {
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 입력장치
	hListBox = GetDlgItem(hwnd, IDC_COMBO_INPUT_DEV); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"speaker");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"mic");
		}
		if (settings.find("input_dev") == settings.end()) settings["input_dev"] = DEFULAT_INPUT_DEV;
		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			SendMessage(hListBox, CB_GETLBTEXT, (WPARAM)i, (LPARAM)tmp);
			if (CompareStringWString(settings.value("input_dev", ""), (WCHAR*)tmp) == true) {
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 원문 자막 표시
	CheckDlgButton(hwnd, IDC_CHECK_VSUB, settings["ck_orgin_subtext"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 자동 번역 PC
	CheckDlgButton(hwnd, IDC_CHECK_PCTRANS, settings["ck_pctrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 자동 번역 API
	CheckDlgButton(hwnd, IDC_CHECK_APITRANS, settings["ck_apitrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// 요약 API
	CheckDlgButton(hwnd, IDC_CHECK_SUMMARY, settings["ck_summary"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);

	// 번역 입력 언어
	hListBox = GetDlgItem(hwnd, IDC_COMBO_VOICE_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			// 언어와 관련 데이터 쌍을 배열로 초기화
			LanguageData languages[] = {
				{L"ALL", L"ALL"}, {L"Korean", L"kor_Hang"}, {L"English", L"eng_Latn"},
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

			// languages 배열을 순회하며 각 언어와 데이터를 hListBox에 추가
			int lenLang = sizeof(languages) / sizeof(languages[0]);
			for (int i = 0; i < lenLang; ++i) {
				// 언어 문자열을 리스트 박스에 추가하고 인덱스를 받음
				index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
				// 얻은 인덱스를 사용하여 해당 언어에 관련 데이터 설정
				SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
			}
		}

		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			LPARAM itemData = SendMessage(hListBox, CB_GETITEMDATA, (WPARAM)i, 0);
			if (CompareStringWString(settings.value("cb_voice_lang", ""), (WCHAR*)itemData) == true) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 번역 결과 언어 ( PC )
	hListBox = GetDlgItem(hwnd, IDC_COMBO_PCTRANS_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			// 언어와 관련 데이터 쌍을 초기화합니다.
			LanguageData languages[] = {
				{L"Korean, Hangul", L"kor_Hang"},
				{L"English, Latin", L"eng_Latn"},
				{L"Acehnese, Arabic", L"ace_Arab"},
				{L"Acehnese, Latin", L"ace_Latn"},
				{L"Mesopotamian Arabic, Arabic", L"acm_Arab"},
				{L"Ta'izzi-Adeni Arabic, Arabic", L"acq_Arab"},
				{L"Tunisian Arabic, Arabic", L"aeb_Arab"},
				{L"Afrikaans, Latin", L"afr_Latn"},
				{L"South Arabian Arabic, Arabic", L"ajp_Arab"},
				{L"Akan, Latin", L"aka_Latn"},
				{L"Amharic, Ethiopic", L"amh_Ethi"},
				{L"Levantine Arabic, Arabic", L"apc_Arab"},
				{L"Standard Arabic, Arabic", L"arb_Arab"},
				{L"Najdi Arabic, Arabic", L"ars_Arab"},
				{L"Moroccan Arabic, Arabic", L"ary_Arab"},
				{L"Egyptian Arabic, Arabic", L"arz_Arab"},
				{L"Assamese, Bengali", L"asm_Beng"},
				{L"Asturian, Latin", L"ast_Latn"},
				{L"Awadhi, Devanagari", L"awa_Deva"},
				{L"Aymara, Latin", L"ayr_Latn"},
				{L"South Azerbaijani, Arabic", L"azb_Arab"},
				{L"North Azerbaijani, Latin", L"azj_Latn"},
				{L"Bashkir, Cyrillic", L"bak_Cyrl"},
				{L"Bambara, Latin", L"bam_Latn"},
				{L"Balinese, Latin", L"ban_Latn"},
				{L"Belarusian, Cyrillic", L"bel_Cyrl"},
				{L"Bemba, Latin", L"bem_Latn"},
				{L"Bengali, Bengali", L"ben_Beng"},
				{L"Bhojpuri, Devanagari", L"bho_Deva"},
				{L"Banjar, Arabic", L"bjn_Arab"},
				{L"Banjar, Latin", L"bjn_Latn"},
				{L"Tibetan, Tibetan", L"bod_Tibt"},
				{L"Bosnian, Latin", L"bos_Latn"},
				{L"Buginese, Latin", L"bug_Latn"},
				{L"Bulgarian, Cyrillic", L"bul_Cyrl"},
				{L"Catalan, Latin", L"cat_Latn"},
				{L"Cebuano, Latin", L"ceb_Latn"},
				{L"Czech, Latin", L"ces_Latn"},
				{L"Chakma, Latin", L"cjk_Latn"},
				{L"Sorani Kurdish, Arabic", L"ckb_Arab"},
				{L"Crimean Tatar, Latin", L"crh_Latn"},
				{L"Welsh, Latin", L"cym_Latn"},
				{L"Danish, Latin", L"dan_Latn"},
				{L"German, Latin", L"deu_Latn"},
				{L"Dinka, Latin", L"dik_Latn"},
				{L"Dyula, Latin", L"dyu_Latn"},
				{L"Dzongkha, Tibetan", L"dzo_Tibt"},
				{L"Greek, Greek", L"ell_Grek"},
				{L"Esperanto, Latin", L"epo_Latn"},
				{L"Estonian, Latin", L"est_Latn"},
				{L"Basque, Latin", L"eus_Latn"},
				{L"Ewe, Latin", L"ewe_Latn"},
				{L"Faroese, Latin", L"fao_Latn"},
				{L"Persian (Farsi), Arabic", L"pes_Arab"},
				{L"Fijian, Latin", L"fij_Latn"},
				{L"Finnish, Latin", L"fin_Latn"},
				{L"Fon, Latin", L"fon_Latn"},
				{L"French, Latin", L"fra_Latn"},
				{L"Friulian, Latin", L"fur_Latn"},
				{L"Fulfulde, Latin", L"fuv_Latn"},
				{L"Scottish Gaelic, Latin", L"gla_Latn"},
				{L"Irish, Latin", L"gle_Latn"},
				{L"Galician, Latin", L"glg_Latn"},
				{L"Guarani, Latin", L"grn_Latn"},
				{L"Gujarati, Gujarati", L"guj_Gujr"},
				{L"Haitian Creole, Latin", L"hat_Latn"},
				{L"Hausa, Latin", L"hau_Latn"},
				{L"Hebrew, Hebrew", L"heb_Hebr"},
				{L"Hindi, Devanagari", L"hin_Deva"},
				{L"Chhattisgarhi, Devanagari", L"hne_Deva"},
				{L"Croatian, Latin", L"hrv_Latn"},
				{L"Hungarian, Latin", L"hun_Latn"},
				{L"Armenian, Armenian", L"hye_Armn"},
				{L"Igbo, Latin", L"ibo_Latn"},
				{L"Ilocano, Latin", L"ilo_Latn"},
				{L"Indonesian, Latin", L"ind_Latn"},
				{L"Icelandic, Latin", L"isl_Latn"},
				{L"Italian, Latin", L"ita_Latn"},
				{L"Javanese, Latin", L"jav_Latn"},
				{L"Japanese, Japanese (Kana,Kanji)", L"jpn_Jpan"},
				{L"Kabyle, Latin", L"kab_Latn"},
				{L"Kachin, Latin", L"kac_Latn"},
				{L"Kamba, Latin", L"kam_Latn"},
				{L"Kannada, Kannada", L"kan_Knda"},
				{L"Kashmiri (Arabic), Arabic", L"kas_Arab"},
				{L"Kashmiri (Devanagari), Devanagari", L"kas_Deva"},
				{L"Georgian, Georgian", L"kat_Geor"},
				{L"Kazakh, Cyrillic", L"kaz_Cyrl"},
				{L"Kabiyè, Latin", L"kbp_Latn"},
				{L"Cape Verdean Creole, Latin", L"kea_Latn"},
				{L"Khmer, Khmer", L"khm_Khmr"},
				{L"Kikuyu, Latin", L"kik_Latn"},
				{L"Kinyarwanda, Latin", L"kin_Latn"},
				{L"Kyrgyz, Cyrillic", L"kir_Cyrl"},
				{L"Kimbundu, Latin", L"kmb_Latn"},
				{L"Kongo, Latin", L"kon_Latn"},
				{L"Northern Kurdish, Latin", L"kmr_Latn"},
				{L"Lao, Lao", L"lao_Laoo"},
				{L"Latin, Latin", L"lat_Latn"},
				{L"Latvian, Latin", L"lav_Latn"},
				{L"Limburgish, Latin", L"lim_Latn"},
				{L"Lingala, Latin", L"lin_Latn"},
				{L"Lithuanian, Latin", L"lit_Latn"},
				{L"Lombard, Latin", L"lmo_Latn"},
				{L"Luxembourgish, Latin", L"ltz_Latn"},
				{L"Luba-Lulua, Latin", L"lua_Latn"},
				{L"Ganda, Latin", L"lug_Latn"},
				{L"Luo, Latin", L"luo_Latn"},
				{L"Mizo, Latin", L"lus_Latn"},
				{L"Malayalam, Malayalam", L"mal_Mlym"},
				{L"Marathi, Devanagari", L"mar_Deva"},
				{L"Macedonian, Cyrillic", L"mkd_Cyrl"},
				{L"Malagasy, Latin", L"mlg_Latn"},
				{L"Maltese, Latin", L"mlt_Latn"},
				{L"Manipuri, Bengali", L"mni_Beng"},
				{L"Mossi, Latin", L"mos_Latn"},
				{L"Maori, Latin", L"mri_Latn"},
				{L"Burmese, Myanmar", L"mya_Mymr"},
				{L"Dutch, Latin", L"nld_Latn"},
				{L"Nynorsk, Latin", L"nno_Latn"},
				{L"Bokmål, Latin", L"nob_Latn"},
				{L"Nepali (Individual), Devanagari", L"npi_Deva"},
				{L"Northern Sotho, Latin", L"nso_Latn"},
				{L"Chichewa, Latin", L"nya_Latn"},
				{L"Occitan, Latin", L"oci_Latn"},
				{L"Oriya, Oriya", L"ory_Orya"},
				{L"Pangasinan, Latin", L"pag_Latn"},
				{L"Punjabi (Gurmukhi), Gurmukhi", L"pan_Guru"},
				{L"Papiamento, Latin", L"pap_Latn"},
				{L"Polish, Latin", L"pol_Latn"},
				{L"Portuguese, Latin", L"por_Latn"},
				{L"Dari, Arabic", L"prs_Arab"},
				{L"Southern Pashto, Arabic", L"pbt_Arab"},
				{L"Quechua, Latin", L"quy_Latn"},
				{L"Romanian, Latin", L"ron_Latn"},
				{L"Rundi, Latin", L"run_Latn"},
				{L"Russian, Cyrillic", L"rus_Cyrl"},
				{L"Sango, Latin", L"sag_Latn"},
				{L"Sanskrit, Devanagari", L"san_Deva"},
				{L"Santali, Bengali", L"sat_Beng"},
				{L"Sicilian, Latin", L"scn_Latn"},
				{L"Shan, Myanmar", L"shn_Mymr"},
				{L"Sinhala, Sinhala", L"sin_Sinh"},
				{L"Slovak, Latin", L"slk_Latn"},
				{L"Slovenian, Latin", L"slv_Latn"},
				{L"Samoan, Latin", L"smo_Latn"},
				{L"Shona, Latin", L"sna_Latn"},
				{L"Sindhi, Arabic", L"snd_Arab"},
				{L"Somali, Latin", L"som_Latn"},
				{L"Sotho, Latin", L"sot_Latn"},
				{L"Spanish, Latin", L"spa_Latn"},
				{L"Sardinian, Latin", L"srd_Latn"},
				{L"Serbian, Cyrillic", L"srp_Cyrl"},
				{L"Swati, Latin", L"ssw_Latn"},
				{L"Sundanese, Latin", L"sun_Latn"},
				{L"Swedish, Latin", L"swe_Latn"},
				{L"Swahili, Latin", L"swh_Latn"},
				{L"Silesian, Latin", L"szl_Latn"},
				{L"Tamil, Tamil", L"tam_Taml"},
				{L"Tatar, Cyrillic", L"tat_Cyrl"},
				{L"Telugu, Telugu", L"tel_Telu"},
				{L"Tajik, Cyrillic", L"tgk_Cyrl"},
				{L"Tagalog, Latin", L"tgl_Latn"},
				{L"Thai, Thai", L"tha_Thai"},
				{L"Tigrinya, Ethiopic", L"tir_Ethi"},
				{L"Tok Pisin, Latin", L"tpi_Latn"},
				{L"Tswana, Latin", L"tsn_Latn"},
				{L"Tsonga, Latin", L"tso_Latn"},
				{L"Turkmen, Latin", L"tuk_Latn"},
				{L"Turkish, Latin", L"tur_Latn" },
				{ L"Twi, Latin", L"twi_Latn" },
				{ L"Uighur, Arabic", L"uig_Arab" },
				{ L"Ukrainian, Cyrillic", L"ukr_Cyrl" },
				{ L"Urdu, Arabic", L"urd_Arab" },
				{ L"Uzbek, Latin", L"uzb_Latn" },
				{ L"Venetian, Latin", L"vec_Latn" },
				{ L"Vietnamese, Latin", L"vie_Latn" },
				{ L"Waray, Latin", L"war_Latn" },
				{ L"Wolof, Latin", L"wol_Latn" },
				{ L"Xhosa, Latin", L"xho_Latn" },
				{ L"Eastern Yiddish, Hebrew", L"ydd_Hebr" },
				{ L"Yoruba, Latin", L"yor_Latn" },
				{ L"Cantonese (Traditional), Traditional Chinese", L"yue_Hant" },
				{ L"Chinese (Simplified), Simplified Chinese", L"zho_Hans" },
				{ L"Chinese (Traditional), Traditional Chinese", L"zho_Hant" },
				{ L"Zulu, Latin", L"zul_Latn" }
			};

			// languages 배열을 순회하며 각 언어와 데이터를 hListBox에 추가
			int lenLang = sizeof(languages) / sizeof(languages[0]);
			for (int i = 0; i < lenLang; ++i) {
				// 언어 문자열을 리스트 박스에 추가하고 인덱스를 받음
				index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)languages[i].language);
				// 얻은 인덱스를 사용하여 해당 언어에 관련 데이터 설정
				SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)languages[i].data);
			}
		}

		//SendMessage(hListBox, CB_SETCURSEL, settings["cb_pctrans_lang"].get<int>(), 0); // 두번째 1,0
		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			LPARAM itemData = SendMessage(hListBox, CB_GETITEMDATA, (WPARAM)i, 0);
			if (CompareStringWString(settings.value("cb_pctrans_lang", ""), (WCHAR*)itemData) == true) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 번역 결과 언어 ( API ) 
	hListBox = GetDlgItem(hwnd, IDC_COMBO_APITRANS_LANG); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			for (int i = 0; i < 32; i++) {
				index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)langArr[i][0]);
				SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)langArr[i][1]);
			}
		}

		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			LPARAM itemData = SendMessage(hListBox, CB_GETITEMDATA, (WPARAM)i, 0);
			if (CompareStringWString(settings.value("cb_apitrans_lang", ""), (WCHAR*)itemData) == true) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}
	}

	// 요약 언어
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUMMARY); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			int lenLang = sizeof(langEnv) / sizeof(langEnv[0]);
			for (int i = 0; i < lenLang; ++i) {
				SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)langEnv[i]);
			}
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_lang"].get<int>(), 0); // 두번째 1,0
	}

	// 번역에 사용할 API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_TRANS_API); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"DeepL");
			SendMessage(hListBox, CB_ADDSTRING, 1, (LPARAM)L"DeepL Pro");
		}

		// 첫 번째 아이템을 선택 상태로 설정
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_trans_api"].get<int>(), 0); // 두번째 1,0
	}

	// 요약에 사용할 API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUM_API); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		if (isStart) {
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
	//wsprintf(tmp, L"%d", settings["cb_oldfont_size"].get<int>());
	//SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_SIZE, tmp);

	// 요약 폰트 사이즈 
	wsprintf(tmp, L"%d", settings["cb_sumfont_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_SIZE, tmp);

	// 투명도
	hListBox = GetDlgItem(hwnd, IDC_COMBO_TRANSPARENT); // ListBox의 핸들을 가져옵니다.
	if (hListBox != NULL) {
		std::string transparentPercent = "";
		if (isStart) {
			// 0 ~ 100까지의 숫자를 추가합니다.
			for (int i = 0; i <= 100;) {
				wsprintf(tmp, L"%d", i);
				SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)tmp);
				i += 10;
			}
		}

		if (settings.find("transparent") == settings.end()) settings["transparent"] = DEFULAT_TRANSPARENT;
		wsprintf(tmp, L"%d", settings["transparent"].get<int>());
		SetDlgItemText(hwnd, IDC_COMBO_TRANSPARENT, tmp);
	}

	// 폰트 컬러
	SetDlgItemText(hwnd, IDC_EDIT_FONT_COLOR, StringToWCHAR(settings.value("ed_font_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_COLOR, StringToWCHAR(settings.value("ed_oldfont_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_COLOR, StringToWCHAR(settings.value("ed_sumfont_color", "")).c_str());

	// 배경 컬러
	if (settings.find("bg_color") == settings.end()) settings["bg_color"] = DEFULAT_BG_COLOR;
	SetDlgItemText(hwnd, IDC_BG_COLOR, StringToWCHAR(settings.value("bg_color", "")).c_str());

	// API Key
	SetDlgItemText(hwnd, IDC_EDIT_TRANS_API_KEY, StringToWCHAR(settings.value("ed_trans_api_key", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_API_KEY, StringToWCHAR(settings.value("ed_summary_api_key", "")).c_str());

	// 요약 힌트
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_HINT, StringToWCHAR(settings.value("ed_summary_hint", "")).c_str());
}

// 초기화
void CSettings::InitSettings(HWND hwnd) {
	// settings에 Default 값 셋팅
	DefaultJson();
	RefreshSettings(hwnd, false);
}

// 설정 파일 읽기
void CSettings::ReadSettings(const std::string& filePath) {
	// 파일에서 JSON 데이터 읽기
	std::string strFileName = RealTrans::GetInstance()->GetExecutePath() + filePath;
	std::ifstream file(strFileName);
	if (file.is_open()) {
		file >> settings;
		file.close();

		// 추가 인자 확인
		if (settings.find("transparent") == settings.end()) settings["transparent"] = DEFULAT_TRANSPARENT;
		if (settings.find("bg_color") == settings.end()) settings["bg_color"] = DEFULAT_BG_COLOR;
	}
	else {
		DefaultJson();
	}
}

// 설정 파일을 읽고 현재 설정에	적용
void CSettings::LoadSettings(HWND hwnd, const std::string& filePath)
{
	ReadSettings(filePath);

	RefreshSettings(hwnd, true);
}

void CSettings::DumpSettings() {
	// json 객체를 문자열로 변환 (들여쓰기 포함)
	std::string dump = settings.dump(4);

	// UTF-8 문자열을 UTF-16으로 변환
	std::wstring wdump = Utf8ToWideString(dump);

	// OutputDebugString으로 출력
	OutputDebugStringW(L"===== Settings Dump =====\n");
	OutputDebugStringW(wdump.c_str());
	OutputDebugStringW(L"\n========================\n");
}

void CSettings::SetFontSize(HWND hEdit) {
	HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
	SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// 정적 다이얼로그 프로시저 - 인스턴스와 연결된 실제 프로시저 호출
INT_PTR CALLBACK CSettings::StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// WM_INITDIALOG 메시지에서 인스턴스 포인터 설정
	if (uMsg == WM_INITDIALOG) {
		// lParam으로 전달된 this 포인터를 hwndDlg의 사용자 데이터로 저장
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		CSettings* pThis = (CSettings*)lParam;
		// 다이얼로그 핸들 저장
		pThis->m_hDlg = hwndDlg;
	}

	// 인스턴스 포인터 가져오기
	CSettings* pThis = (CSettings*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	// 인스턴스가 있으면 해당 인스턴스의 DialogProc 호출
	if (pThis) {
		return pThis->DialogProc(hwndDlg, uMsg, wParam, lParam);
	}

	return FALSE;
}

// 인스턴스 다이얼로그 프로시저
INT_PTR CSettings::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		// 다이얼로그 초기화 코드 시작
		LoadSettings(hwndDlg, "config.json");
		SetFontSize(GetDlgItem(hwndDlg, IDC_EDIT_SUMMARY_HINT));
		SetFontSize(GetDlgItem(hwndDlg, IDC_EDIT_TRANS_API_KEY));
		SetFontSize(GetDlgItem(hwndDlg, IDC_EDIT_SUMMARY_API_KEY));

		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDCANCEL: // 취소
			EndDialog(hwndDlg, LOWORD(wParam));
			m_hDlg = NULL;
			RealTrans::GetInstance()->SetSettingStatus(false);
			return (INT_PTR)TRUE;
		case IDSUPPLY: // 저장 및 종료
			SaveSettings(hwndDlg, "config.json");
			RealTrans::GetInstance()->SetNemoViewerConfig();
			EndDialog(hwndDlg, LOWORD(wParam));
			m_hDlg = NULL;
			RealTrans::GetInstance()->SetSettingStatus(false);
			return (INT_PTR)TRUE;
		case ID_INIT: // 초기화
			if (wmEvent == BN_CLICKED)
				InitSettings(hwndDlg);
			break;

		case IDC_CHECK_PCTRANS:
			if (wmEvent == BN_CLICKED) {
				// PC 번역 체크박스 처리 로직
				//bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PCTRANS) == BST_CHECKED);
				// 필요한 처리를 여기에 구현
			}
			break;
		case IDC_CHECK_APITRANS:
			if (wmEvent == BN_CLICKED) {
				// API 번역 체크박스 처리 로직
				//bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_APITRANS) == BST_CHECKED);
				// 필요한 처리를 여기에 구현
			}
			break;
		case IDC_COMBO_MODEL_SIZE:
		case IDC_COMBO_PROC:
		case IDC_COMBO_INPUT_DEV:
			if (wmEvent == CBN_SELCHANGE) {
				// 콤보박스 선택 변경 처리
				TCHAR szLoadedString[256];
				TCHAR szLoadedStringALERT[256];

				HINSTANCE hInst = GetModuleHandle(NULL);
				LoadStringW(hInst, IDS_ALERT, szLoadedStringALERT, 256);
				if (LoadStringW(hInst, IDS_MODEL_CHANGE_WARNING, szLoadedString, 256) > 0) {
					MessageBox(hwndDlg, szLoadedString, szLoadedStringALERT, MB_OK | MB_ICONWARNING);
				}
			}
			break;

		}
	}
	break;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		m_hDlg = NULL;
		RealTrans::GetInstance()->SetSettingStatus(false);
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

// 글로벌 다이얼로그 프로시저 함수 (외부에서 접근하기 위한 함수)
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return CSettings::StaticDialogProc(hwndDlg, uMsg, wParam, lParam);
}