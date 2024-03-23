#include "CSettings.h"
#include <fstream>
#include <iostream>
#include <stdexcept> // std::invalid_argument, std::out_of_range 예외를 처리하기 위해 포함
#include "Resource.h"
#include "json.hpp" // 또는 경로를 지정해야 할 경우: #include "External/json.hpp"
#include "StrConvert.h"
#include "CDlgSummary.h"
#include "language.h"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

// 환경 설정을 위한 JSON 객체 생성
extern json settings;
extern bool isRefreshEnv;
extern time_t nSummaryTime;
extern CDlgSummary* DlgSum;

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

// Python 프로그램에 전달할 명령을 pymsg.json 파일로 저장하는 함수 : Python 프로그램은 pymsg.json 파일을 읽어서 명령을 수행
void MakeChildCmd(const std::string& src_lang, const std::string& tgt_lang )
{
	std::string txt = "{ \"src_lang\": \"" + src_lang + "\", \"tgt_lang\": \"" + tgt_lang + "\" }";
	std::ofstream file("pymsg.json");
	file << txt;
}

// 현재 설정을 파일로 저장
void SaveSettings(HWND hwnd, const std::string& filePath) {
	BOOL old_pctrans = settings["ck_pctrans"];
	LRESULT nSel = 0;
	LPARAM itemData;
	HWND hCbBox;
	WCHAR comboBoxItem[256];

	hCbBox = GetDlgItem(hwnd, IDC_COMBO_UI_LANG);
	settings["ui_lang"] = SendMessage(hCbBox, CB_GETCURSEL, 0, 0);

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
	settings["ui_lang"] = 0; // 언어
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
	int index;
	int itemCount;
	HWND hListBox;
	//json langMap;

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

		// 첫 번째 아이템을 선택 상태로 설정
		//SendMessage(hListBox, CB_SETCURSEL, settings["cb_voice_lang"].get<int>(), 0); // 두번째 1,0
		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			LPARAM itemData = SendMessage(hListBox, CB_GETITEMDATA, (WPARAM)i, 0);
			//if (settings["cb_voice_lang"] == itemData ) {
			if (CompareStringWString(settings.value("cb_voice_lang", ""),(WCHAR*)itemData)==true) {
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
			//if (settings["cb_pctrans_lang"] == itemData) {
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
			for (int i = 0; i < 32;i++) {
				index = SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)langArr[i][0]);
				SendMessage(hListBox, CB_SETITEMDATA, (WPARAM)index, (LPARAM)langArr[i][1]);
			}
		}

		itemCount = SendMessage(hListBox, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; ++i) {
			LPARAM itemData = SendMessage(hListBox, CB_GETITEMDATA, (WPARAM)i, 0);
			if (CompareStringWString(settings.value("cb_apitrans_lang", ""), (WCHAR*)itemData) == true) {
				// 원하는 데이터를 가진 항목을 찾았으므로, 해당 항목을 선택합니다.
				//DlgSum->Alert((WCHAR*)itemData);
				SendMessage(hListBox, CB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}

		// 첫 번째 아이템을 선택 상태로 설정
		//SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_lang"].get<int>(), 0); // 두번째 1,0
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
					//bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PCTRANS) == BST_CHECKED);
					//if (checked) {
					//	// 첫 번째 체크박스가 체크되어 있으면, 두 번째 체크박스를 체크 해제합니다. : 중복사용을 위해 막음
					//	//CheckDlgButton(hwndDlg, IDC_CHECK_APITRANS, BST_UNCHECKED);
					//}
				}
				break;
			case IDC_CHECK_APITRANS:
				if (wmEvent == BN_CLICKED) {
					// 체크박스 상태 읽기
					//bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_APITRANS) == BST_CHECKED);
					//if (checked) {
					//	// 첫 번째 체크박스가 체크되어 있으면, 두 번째 체크박스를 체크 해제합니다. : 중복사용을 위해 막음
					//	//CheckDlgButton(hwndDlg, IDC_CHECK_PCTRANS, BST_UNCHECKED);
					//}
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