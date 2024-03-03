#include "CSettings.h"
#include <fstream>
#include <iostream>
#include <stdexcept> // std::invalid_argument, std::out_of_range ���ܸ� ó���ϱ� ���� ����
#include "Resource.h"
#include "json.hpp" // �Ǵ� ��θ� �����ؾ� �� ���: #include "External/json.hpp"
#include "StrConvert.h"
#include "CDlgSummary.h"

// nlohmann ���̺귯���� ���ӽ����̽� ���
using json = nlohmann::json;

// ȯ�� ������ ���� JSON ��ü ����
extern json settings;
extern BOOL isRefreshEnv;
extern time_t nSummaryTime;
extern CDlgSummary* DlgSum;

// JSON ��ü�� ���ڿ��� ����ϴ� �Լ�
void PrintJson()
{
	// JSON ��ü ���� �� ��� ����
	json j;
	j["name"] = "John";
	j["age"] = 30;
	j["is_student"] = false;
	j["skills"] = { "C++", "Python", "JavaScript" };

	// JSON ��ü�� ���ڿ��� ���
	//std::cout << j.dump(4) << std::endl;
}

// Python ���α׷��� ������ ����� childcmd.temp ���Ϸ� �����ϴ� �Լ� : Python ���α׷��� childcmd.temp ������ �о ����� ����
void MakeChildCmd(const char* cmd)
{
	std::ofstream file("childcmd.temp");
	file << cmd;
}

// ���� ������ ���Ϸ� ����
void SaveSettings(HWND hwnd, const std::string& filePath) {
	BOOL old_pctrans = settings["ck_pctrans"];

	// Checkbox ���� �б� (����: üũ�ڽ��� ID�� IDC_CHECKBOX1�̶�� ����)
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

	// Combobox ���õ� ������ �б� (����: �޺��ڽ��� ID�� IDC_COMBOBOX1�̶�� ����)
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

	// ��� ��Ʈ
	WCHAR lpString[512];
	GetDlgItemText(hwnd, IDC_EDIT_SUMMARY_HINT, lpString, 512);
	settings["ed_summary_hint"] = WCHARToString(lpString);

	// ��� ���Žð� �ʱ�ȭ
	nSummaryTime = 0;

	// ��Ʈ �ʱ�ȭ
	DlgSum->SetFont();

	// JSON ���Ϸ� ����
	std::ofstream file(filePath);
	file << settings.dump(4); // ���ڰ� ����ϱ� ���� 4�� �鿩����� ���
}

// JSON ��ü�� �⺻�� ���� : �ʱ�ȭ �ÿ��� ���
void defaultJson() {
	settings["ck_orgin_subtext"] = false; // ���� �ڸ� ǥ��
	settings["ck_pctrans"] = true; // �ڵ� ���� PC
	settings["ck_apitrans"] = false; // �ڵ� ���� API
	settings["ck_summary"] = false; // ��� API ���

	settings["cb_voice_lang"] = 0; // ���� ���
	settings["cb_pctrans_lang"] = 0; // ���� ��� ��� ( PC )
	settings["cb_summary_lang"] = 0; // ��� ���
	settings["cb_apitrans_sec"] = 1000; // API ���� �ֱ� : Sec
	settings["cb_apitrans_lang"] = 0; // ���� ��� ��� ( API )
	settings["cb_trans_api"] = 0; // ������ ����� API
	settings["cb_summary_min"] = 1; // ��� �ֱ� : Minute
	settings["cb_summary_api"] = 0; // ��࿡ ����� API

	// ��Ʈ �÷�
	settings["ed_font_color"] = "#C8C8FF";
	settings["ed_oldfont_color"] = "#8C8C8C";
	settings["cb_font_size"] = 20;
	settings["cb_oldfont_size"] = 20;
	// ���â ��Ʈ �÷�
	settings["ed_sumfont_color"] = "#C8C8FF";
	settings["cb_sumfont_size"] = 13;

	settings["ed_trans_api_key"] = ""; // ������ ����� API Key
	settings["ed_summary_api_key"] = ""; // ��࿡ ����� API Key
	settings["ed_summary_hint"] = "The following text is a conversation related to securities and economics. Please summarize the main topics, key data points, and conclusions in around 300 words. The summary should be arrange them in number list form to keep them short and concise."; // ��� ��Ʈ
}

// ���� ȭ�� ����
void RefreshSettings(HWND hwnd, BOOL isStart)
{
	WCHAR tmp[10];

	// ���� �ڸ� ǥ��
	CheckDlgButton(hwnd, IDC_CHECK_VSUB, settings["ck_orgin_subtext"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// �ڵ� ���� PC
	CheckDlgButton(hwnd, IDC_CHECK_PCTRANS, settings["ck_pctrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// �ڵ� ���� API
	CheckDlgButton(hwnd, IDC_CHECK_APITRANS, settings["ck_apitrans"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);
	// ��� API
	CheckDlgButton(hwnd, IDC_CHECK_SUMMARY, settings["ck_summary"].get<bool>() ? BST_CHECKED : BST_UNCHECKED);

	// ���� �Է� ���
	HWND hListBox = GetDlgItem(hwnd, IDC_COMBO_VOICE_LANG); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_voice_lang"].get<int>(), 0); // �ι�° 1,0
	}

	// ���� ��� ��� ( PC )
	hListBox = GetDlgItem(hwnd, IDC_COMBO_PCTRANS_LANG); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_pctrans_lang"].get<int>(), 0); // �ι�° 1,0
	}

	// ��� ���
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUMMARY); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_lang"].get<int>(), 0); // �ι�° 1,0
	}

	// ���� ��� ��� ( API )
	hListBox = GetDlgItem(hwnd, IDC_COMBO_APITRANS_LANG); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Korean");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"English");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_apitrans_lang"].get<int>(), 0); // �ι�° 1,0
	}

	// ������ ����� API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_TRANS_API); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"DeepL");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_trans_api"].get<int>(), 0); // �ι�° 1,0
	}

	// ��࿡ ����� API
	hListBox = GetDlgItem(hwnd, IDC_COMBO_SUM_API); // ListBox�� �ڵ��� �����ɴϴ�.
	if (hListBox != NULL) {
		if (isStart) {
			//SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"Google");
			SendMessage(hListBox, CB_ADDSTRING, 0, (LPARAM)L"OpenAI");
		}

		// ù ��° �������� ���� ���·� ����
		SendMessage(hListBox, CB_SETCURSEL, settings["cb_summary_api"].get<int>(), 0); // �ι�° 1,0
	}

	// ��� �ֱ� : Minute
	wsprintf(tmp, L"%d", settings["cb_summary_min"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_MIN, tmp);

	// ��Ʈ ������ 
	wsprintf(tmp, L"%d", settings["cb_font_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_FONT_SIZE, tmp);

	// ������ ��Ʈ ������ 
	wsprintf(tmp, L"%d", settings["cb_oldfont_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_SIZE, tmp);

	// ��� ��Ʈ ������ 
	wsprintf(tmp, L"%d", settings["cb_sumfont_size"].get<int>());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_SIZE, tmp);

	// ��Ʈ �÷�
	SetDlgItemText(hwnd, IDC_EDIT_FONT_COLOR, StringToWCHAR(settings.value("ed_font_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_OLDFONT_COLOR, StringToWCHAR(settings.value("ed_oldfont_color", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMFONT_COLOR, StringToWCHAR(settings.value("ed_sumfont_color", "")).c_str());

	// API Key
	SetDlgItemText(hwnd, IDC_EDIT_TRANS_API_KEY, StringToWCHAR(settings.value("ed_trans_api_key", "")).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_API_KEY, StringToWCHAR(settings.value("ed_summary_api_key", "")).c_str());

	// ��� ��Ʈ
	SetDlgItemText(hwnd, IDC_EDIT_SUMMARY_HINT, StringToWCHAR(settings.value("ed_summary_hint", "")).c_str());
}

// �ʱ�ȭ
void InitSettings(HWND hwnd) {
	// settings�� Default �� ����
	defaultJson();
	RefreshSettings(hwnd, false);
}

// ���� ���� �б�
void ReadSettings(const std::string& filePath) {
	// ���Ͽ��� JSON ������ �б�
	std::ifstream file(filePath);
	if (file.is_open()) {
		file >> settings;
		//std::cout << "�Է� ���� ���ڿ� :: " << file.dump() << std::endl;
	}
	else {
		defaultJson();
	}
}

// ���� ������ �а� ���� ������	����
void LoadSettings(HWND hwnd, const std::string& filePath)
{
	ReadSettings(filePath);

	RefreshSettings(hwnd, true);
}

// Setting Dialog �޽��� ó��
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	//if (GetCurrentThreadId() != g_MainUIThreadID) return (INT_PTR)FALSE;

	switch (uMsg) {
	case WM_INITDIALOG:
	{
		// ���̾�α� �ʱ�ȭ �ڵ� ����
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
			case IDCANCEL: // ���
				EndDialog(hwndDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			case IDSUPPLY: // ���� �� ����
				SaveSettings(hwndDlg, "config.json");
				isRefreshEnv = true;
				EndDialog(hwndDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			case ID_INIT: // �ʱ�ȭ
				if (wmEvent == BN_CLICKED)
					InitSettings(hwndDlg);
				//return (INT_PTR)TRUE;
				break;

			case IDC_CHECK_PCTRANS:
				if (wmEvent == BN_CLICKED) {
					// üũ�ڽ� ���� �б�
					bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PCTRANS) == BST_CHECKED);
					if (checked) {
						// ù ��° üũ�ڽ��� üũ�Ǿ� ������, �� ��° üũ�ڽ��� üũ �����մϴ�. : �ߺ������ ���� ����
						//CheckDlgButton(hwndDlg, IDC_CHECK_APITRANS, BST_UNCHECKED);
					}
				}
				break;
			case IDC_CHECK_APITRANS:
				if (wmEvent == BN_CLICKED) {
					// üũ�ڽ� ���� �б�
					bool checked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_APITRANS) == BST_CHECKED);
					if (checked) {
						// ù ��° üũ�ڽ��� üũ�Ǿ� ������, �� ��° üũ�ڽ��� üũ �����մϴ�. : �ߺ������ ���� ����
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