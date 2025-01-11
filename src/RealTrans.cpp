// RealTrans.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

//#include "framework.h"
#include "RealTrans.h"
//#include <CommCtrl.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <atlstr.h>
//#include <stdlib.h>
#include <Richedit.h>
#include <shellapi.h>
#include <WinUser.h>
#include <tlhelp32.h>
#include <mutex>
#include <windef.h>
#include "SoundDev.h"
#include "CMenusDlg.h"
#include "CSettings.h"
#include "resource.h"
#include "json.hpp"
#include "CDlgSummary.h"
#include "CDlgInfo.h"
#include "CDlgTrans.h"
#include "StrConvert.h"
#include "deepl_api.h"
#include "language.h"
#include "version.h"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

#define MAX_LOADSTRING 100
#define TIMER_ID 1 // 타이머 ID

// 전역 변수로 타이틀바 아이콘에 대한 식별자와 위치를 정의
#define IDI_SETTINGS 132
RECT iconRect;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
std::string strExecutePath = "";

HWND hwndTextBox; // 전역 변수로 메인 윈도우 핸들과 텍스트박스 핸들을 선언합니다.
HFONT hFont; // 전역 변수로 폰트 핸들을 저장
PROCESS_INFORMATION piProcInfo; // child 프로세스 정보, 종료시에 kill해주기위해서 전역으로 설정

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 비동기 읽기를 위한 오버랩 구조체 준비
OVERLAPPED g_Overlapped = { 0 };

// 읽기 완료 이벤트 핸들
HANDLE g_hReadEvent;
HANDLE hChildStdOutRead = NULL; // Read-side, used in calls to ReadFile() to get child's stdout output.
HANDLE hChildStdOutWrite = NULL; // Write-side, given to child process using si struct.

CString strResult; // Contains result of cmdArg.

// Left Menu
CMenusDlg* lmenu;
BOOL bStopMenu = FALSE;

// Thread
HANDLE hThread;
HANDLE hBgThread;
// 글로벌 변수로 메인 UI 스레드 ID 저장
DWORD g_MainUIThreadID;
DWORD g_WorkThreadID;
std::mutex mtx; // 공유 자원에 대한 뮤텍스

//char addText[4096] = "";
std::string addText = "";
bool addFlag = false;
std::string strEng = "";

// 환경설정
bool isRefreshEnv = false;
json settings;

// 요약창
CDlgSummary* DlgSum;
time_t nSummaryTime = 0;
bool runSummary = false;
extern bool addSummaryFlag;

// 정보창
CDlgInfo* DlgInfo = NULL;

// 번역창
CDlgTrans* DlgTrans = NULL;

// 언어
LPWSTR oldUILang;

void DebugHandle(HWND value)
{
	WCHAR buf[200];
	wsprintf(buf, L"Handle: %d", value);
	SendMessage(hwndTextBox, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
}

void DebugText(const WCHAR* text)
{
	SendMessage(hwndTextBox, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)text);
}

COLORREF HexToCOLORREF(const std::string& hexCode) {
	// 색상 코드에서 '#' 문자를 제거합니다.
	std::string hex = hexCode.substr(1);

	// 16진수 값을 10진수로 변환합니다.
	int r = std::stoi(hex.substr(0, 2), nullptr, 16);
	int g = std::stoi(hex.substr(2, 2), nullptr, 16);
	int b = std::stoi(hex.substr(4, 2), nullptr, 16);

	// COLORREF 값으로 변환합니다. RGB 매크로를 사용합니다.
	return RGB(r, g, b);
}

// 제일 마지막 줄이 보이도록
void ScrollToBottom(HWND hRichEdit) {
	int totalLines = SendMessage(hRichEdit, EM_GETLINECOUNT, 0, 0);
	SendMessage(hRichEdit, EM_LINESCROLL, 0, totalLines);
}

void ScrollToBottomSel(HWND hRichEdit) {
	int textLength = GetWindowTextLength(hRichEdit);

	SetFocus(hRichEdit);

	// 커서를 문서의 끝으로 이동합니다.
	SendMessage(hRichEdit, EM_SETSEL, textLength, textLength);

	// 선택을 취소하고 싶다면 다음과 같이 합니다. 이는 커서를 문서의 끝으로 이동시킨 후 선택 영역이 없도록 합니다.
	SendMessage(hRichEdit, EM_SETSEL, -1, 0);
}

// 지난 글 폰트 색상과 사이즈를 변경하는 함수
void SetRichEditTextColor(HWND hRichEdit) {
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR; // 폰트 크기와 색상 변경을 나타냅니다.
	cf.yHeight = settings["cb_oldfont_size"] * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환
	cf.crTextColor = HexToCOLORREF(settings["ed_oldfont_color"]); // 원하는 색상을 RGB 값으로 설정합니다.

	// 폰트 색상 변경을 적용합니다.
	SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

// 최신 입력글 색상과 사이즈를 변경하는 함수
void SetRichEditSelColor(HWND hRichEdit) {
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_SIZE | CFM_COLOR; // 폰트 크기와 색상 변경을 나타냅니다.
	cf.yHeight = settings["cb_font_size"] * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환
	cf.crTextColor = HexToCOLORREF(settings["ed_font_color"]); // 원하는 색상을 RGB 값으로 설정합니다.

	// 폰트 색상 변경을 적용합니다.
	SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// RichEdit 컨트롤에 텍스트를 추가하는 함수
void AppendTextToRichEdit(HWND hRichEdit, WCHAR* textToAdd) {
	std::wstring wstrAdd = L""; //  L"\r\n";
	wstrAdd += textToAdd;
	// 현재 선택을 문서의 끝으로 설정하여 텍스트 추가 위치를 문서 끝으로 합니다.
	// 이는 텍스트 선택이 없을 경우 커서 위치를 문서 끝으로 이동시키는 효과가 있습니다.
	SetRichEditTextColor(hRichEdit);
	SendMessage(hRichEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

	// 커서 위치(현재 선택의 끝)를 얻습니다.
	DWORD start = 0, end = 0;
	SendMessage(hRichEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

	SetRichEditSelColor(hRichEdit);
	SETTEXTEX st;
	st.flags = ST_DEFAULT; // 기본 설정 사용
	st.codepage = 1200;    // UTF-16LE 코드 페이지
	SendMessage(hRichEdit, EM_REPLACESEL, (WPARAM)&st, (LPARAM)wstrAdd.c_str());

	SendMessage(hRichEdit, EM_SCROLL, SB_BOTTOM, 0);
}

void CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime);

void RunTimer() {
	UINT_PTR timerId = 1; // 타이머 ID
	UINT elapse = 20; // ms

	// 타이머 설정
	if (SetTimer(NULL, timerId, elapse, (TIMERPROC)TimerProc) == 0) {
		std::cerr << "Failed to set timer" << std::endl;
		return;
	}
}

void CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime)
{
	WCHAR strT[4096];

	if (addFlag && addText.size() > 0) {
		if (addText.size() <= 2) {
			mtx.lock(); // 뮤텍스 잠금
			addText.clear();
			addFlag = false;
			mtx.unlock(); // 뮤텍스 해제
			return;
		}

		std::wstring convertBuf;
		mtx.lock(); // 뮤텍스 잠금
		std::string readBuf = addText;
		addText.clear();
		addFlag = false;
		mtx.unlock(); // 뮤텍스 해제
		if (0) {
			convertBuf = Utf8ToWideString(readBuf);
			if (convertBuf.size() > 0) {
				AppendTextToRichEdit(hwndTextBox, (WCHAR*)convertBuf.c_str());
			}
		}
		if (readBuf[1] == '-') {
			if (settings["ck_orgin_subtext"] == true) {
				convertBuf = Utf8ToWideString(readBuf);
				if (convertBuf.size() > 0) {
					AppendTextToRichEdit(hwndTextBox, (WCHAR *)convertBuf.c_str());
				}
			}
			if (settings["ck_apitrans"] == true) {
				// API 번역
				if (settings["cb_trans_api"] == 0) // DeepL
				{
					std::string strOut;

					// 디버깅용 - start
					/*std::string strDebug;
					strDebug = "lang: " + settings.value("cb_apitrans_lang", "");
					strDebug +=", strIn: " + strIn + "\n";
					CharToWChar(strDebug.c_str(), strT);
					AppendTextToRichEdit(hwndTextBox, strT);*/
					// 디버깅용 - end
					RequestDeepL(&readBuf[1], strOut, settings["cb_apitrans_lang"], settings["ed_trans_api_key"], 1);
					std::wstring strChg = L"\r\n";
					strChg += StringToWCHAR(strOut);
					AppendTextToRichEdit(hwndTextBox, (WCHAR *)strChg.c_str());
				}
				else if (settings["cb_trans_api"] == 1) // DeepL Pro
				{
					std::string strOut;
					RequestDeepL(&readBuf[1], strOut, settings["cb_apitrans_lang"], settings["ed_trans_api_key"], 2);
					std::wstring strChg = L"\r\n";
					strChg += StringToWCHAR(strOut);
					AppendTextToRichEdit(hwndTextBox, (WCHAR*)strChg.c_str());
				}
			}
		}
		// 번역 진행도
		else if (readBuf[1] == '[') {
			if (DlgTrans != NULL && DlgTrans->isTrans==TRUE ) {
				// ']'를 찾아서 그 앞까지 읽어서 Percent를 만든다.
				std::string strPercent = readBuf.substr(2, readBuf.find(']'));
				int nPercent = atoi(strPercent.c_str());
				DlgTrans->transPercent = nPercent;
			}
		}
		else {
			if (settings["ck_pctrans"] == true || readBuf[1] == '#') {
				convertBuf = Utf8ToWideString(readBuf);
				if (convertBuf.size() > 0) {
					AppendTextToRichEdit(hwndTextBox, (WCHAR*)convertBuf.c_str());
				}
			}
		}

		convertBuf.clear();
	}

	// 요약창에 추가할 것이 있으면 처리
	if (addSummaryFlag == true)
		DlgSum->AddRichText();
}

void RunJobs() {
	// 자동요약 처리
	if (settings["ck_summary"] == true) {
		time_t now = time(0);
		// 초기 요약시간 설정
		if (nSummaryTime == 0) nSummaryTime = now + settings["cb_summary_min"] * 60;

		if (nSummaryTime < now) {
			// 요약창의 요약버튼 클릭이벤트 발생
			nSummaryTime = now + settings["cb_summary_min"] * 60;
			runSummary = true;
		}
	}
	// 요약 실행
	if (runSummary == true) {
		DlgSum->BgSummary();
		runSummary = false;
	}
	// 번역파일 처리 : translate_out.txt 파일이 있으면 번역창에 표시하고 삭제한다.
	if (DlgTrans != NULL && DlgTrans->bVisible) {
		DlgTrans->CheckTransFile();
		DlgTrans->ProgressBar(DlgTrans->transPercent);
	}

}

// 작업 Thread : Python 프로그램과 pipe로 번역된 결과를 받아서 내부에 적재
DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	// sjheo 동기 읽기 시작
	DWORD dwRead;
	CHAR chBuf[4097];
	std::string strReadBuf;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Thread ID 저장
	g_WorkThreadID = GetCurrentThreadId(); // Work 스레드 ID 저장

	// 표준 출력 파이프 생성
	if (!CreatePipe(&hChildStdOutRead, &hChildStdOutWrite, &saAttr, 0)) {
		std::cerr << "CreatePipe failed\n";
		return 1;
	}

	// 파이프 핸들 상속 불가 설정
	//SetHandleInformation(hChildStdOutRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO siStartInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hChildStdOutWrite;
	siStartInfo.hStdOutput = hChildStdOutWrite;
	siStartInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; // STARTF_USESTDHANDLES is Required.
	siStartInfo.wShowWindow = SW_HIDE; // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

	std::string strProc;
	std::wstring wstrProc;

	strProc = "-d " + settings.value("input_dev", "") + " -p \"" + settings.value("proc", "cuda float16")+"\"";
	wstrProc = StringToWCHAR(strProc);

	std::string strLang;
	std::wstring wstrLang;

	if (settings["ck_pctrans"] == true)
		strLang += "-s " + settings.value("cb_voice_lang", "") + " -t " + settings.value("cb_pctrans_lang", "");
	else  strLang += "-s " + settings.value("cb_voice_lang", "") + " -t xx";

	// 언어 모델 크기 설정
	strLang += " -m " + settings.value("model_size", "");
	wstrLang = StringToWCHAR(strLang);

	// 외부 프로그램 실행 : python 번역 프로그램 ( runTransWin.py)
	TCHAR cmd[300]; // 실행 명령
#ifndef NDEBUG // 디버그 모드
	//swprintf(cmd, 300, L"python.exe D:\\work\\trans\\runTransWin.py -d \"%s\" -v debug %s", strActSound, wstrLang.c_str()); # 언어 체크
	swprintf(cmd, 300, L"python.exe D:\\work\\trans\\runTransWin.py %s %s", wstrProc.c_str(), wstrLang.c_str());
	//swprintf(cmd, 300, L"python.exe D:\\git\\realtrans\\release\\2.0\\runTransWin.py -d \"%s\" -v debug %s", strActSound, wstrLang.c_str()); // git test
	// cmd 내용을 messagebox로 출력
	//MessageBox(NULL, cmd, L"cmd", MB_OK);
#else
	//swprintf(cmd, 300, L"python.exe -W ignore::UserWarning: runTransWin.py -d \"%s\" %s %s", strActSound, wstrLang.c_str(), strSoundInfo);
	//swprintf(cmd, 300, L"python.exe runTransWin.py -d \"%s\" %s %s", strActSound, wstrLang.c_str(), strSoundInfo);
	swprintf(cmd, 300, L"python.exe runTransWin.py %s %s", wstrProc.c_str(), wstrLang.c_str());
#endif
	if (!CreateProcess(

		NULL,             // 실행 파일 경로
		cmd,              // 명령줄 인수 (없으면 NULL)
		NULL,             // 프로세스 핸들 상속 옵션
		NULL,             // 스레드 핸들 상속 옵션
		TRUE,            // 핸들 상속 옵션 ex) TRUE
		CREATE_NEW_CONSOLE, // 새 콘솔 창 생성 옵션 ex) CREATE_NO_WINDOW
		NULL,              // 환경 변수 (없으면 NULL)
		NULL,              // 현재 디렉터리 (없으면 NULL)
		&siStartInfo,     // STARTUPINFO 구조체
		&piProcInfo       // PROCESS_INFORMATION 구조체
	))
	{
		std::cerr << "CreateProcess failed\n";
		return 1;
	}

	// Close the write end of the pipe before reading from the read end of the pipe.
	if (!CloseHandle(hChildStdOutWrite))
	{
		return 1;
	}

	// 처음 입력줄을 skip할 경우 : 처음 2줄은 무시한다.
	int skip_line = 0;

	for (;;)
	{
		dwRead = 0;
		// Read from pipe that is the standard output for child process.
		bool done = !ReadFile(hChildStdOutRead, chBuf, 4096, &dwRead, NULL) || dwRead == 0;

		// Append result to string.
		if (dwRead > 0) {
			chBuf[dwRead] = '\0';
			strReadBuf += chBuf;
			strReadBuf = ConvertBinaryToString(strReadBuf);
			auto end_pos = std::remove(strReadBuf.begin(), strReadBuf.end(), '\n');
			strReadBuf.erase(end_pos, strReadBuf.end());
			if (strReadBuf.size() > 1) {
				mtx.lock(); // 뮤텍스 잠금
				addText += '\n';
				addText += strReadBuf;
				addFlag = true;
				mtx.unlock(); // 뮤텍스 해제
			}
		}
		else chBuf[0] = '\0';

		// 원문 영어 자막
		if (strReadBuf.size() > 1 && strReadBuf[0] == '-') {
			strEng += strReadBuf;
			strEng += "\n";
		}
		
		strReadBuf.clear();
		Sleep(5);
	}
	// sjheo 동기 읽기 종료

	return 0; // 스레드 종료
}

// 작업 Thread가 Sync로 읽어오기때문에 부수적으로 작업할 것들을 처리하기 위해 만듬. : api 통신처리. 
DWORD WINAPI BgProc(LPVOID lpParam)
{
	while (1) {
		// Background Job 처리
		RunJobs();

		Sleep(100);
	}

	return 0;
}


// 번역 콘솔 프로세스 종료
bool TerminatePythonProcess() {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to create process snapshot." << std::endl;
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) {
		std::cerr << "Failed to get the first process." << std::endl;
		CloseHandle(hSnapshot);
		return false;
	}

	bool processTerminated = false;
	do {
		// 여기서는 "python.exe" 또는 "pythonw.exe" 프로세스를 찾습니다.
		if (wcscmp(pe32.szExeFile, L"runTransWin.exe") == 0 || wcscmp(pe32.szExeFile, L"python.exe") == 0) {
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
			if (hProcess != NULL) {
				if (TerminateProcess(hProcess, 0)) {
					std::cout << "runTransWin process terminated successfully." << std::endl;
					processTerminated = true;
				}
				else {
					std::cerr << "Failed to terminate python process." << std::endl;
				}
				CloseHandle(hProcess);
			}
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return processTerminated;
}

BOOL bActThread = FALSE; // 작업 Thread가 실행중인지 확인
BOOL bBgThread = FALSE; // 백그라운드 Thread가 실행중인지 확인
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 현재 실행 경로를 얻어옵니다.
	WCHAR strPath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, strPath, MAX_PATH);
	std::wstring wstrPath = strPath;
	char chPath[MAX_PATH];
	UTF16toUTF8(wstrPath, chPath);
	strExecutePath = chPath;
#ifndef NDEBUG // 디버그 모드
	for (int i = 0; i < 3; i++) {
		strExecutePath = strExecutePath.substr(0, strExecutePath.find_last_of("\\")); // 실행파일 끊고, 실행경로에서 2단계 위로 올라가야함
	}
#else
	for (int i = 0; i < 1; i++) {
		strExecutePath = strExecutePath.substr(0, strExecutePath.find_last_of("\\")); // 실행파일만 끊음.
	}
#endif
	strExecutePath = strExecutePath + "\\";
	SetCurrentDirectory(StringToWString(strExecutePath).c_str());

	// Python 프로세스 종료
	if (!TerminatePythonProcess()) {
		std::cout << "No Python process was terminated." << std::endl;
	}

	// Debug용 콘솔 생성 : 디버깅시에만 사용 ( 사용이 안되어서 다른 방법 사용중 )
	//if (0) {
	//    AllocConsole();

	//    FILE* stream;
	//    // 표준 출력 리디렉션
	//    freopen_s(&stream, "CONOUT$", "w", stdout);
	//    // 표준 에러 리디렉션
	//    freopen_s(&stream, "CONOUT$", "w", stderr);
	//    // 표준 입력 리디렉션
	//    freopen_s(&stream, "CONIN$", "r", stdin);

	//    std::cout << "Hello, World!" << std::endl;
	//}

	// 환경설정값 불러오기
	ReadSettings("config.json");

	GetUserLocale(&oldUILang); // 현재 사용자 인터페이스 언어 설정
	if (settings["ui_lang"] == 0) {
		WCHAR strTmp[][6] = { L"ko-KR", L"en-US", L"de-DE", L"es-ES", L"fr-FR", L"it-IT", L"ja-JP", L"pt-BR", L"ru-RU", L"zh-CN", L"ar-SA", L"hi-IN" };
		int size = 12;
		// 문자열이 strTmp에 존재하는지 확인
		bool exists = std::any_of(std::begin(strTmp), std::end(strTmp), [&](const WCHAR* s) { return lstrcmpW(s, oldUILang) == 0; });
		if (exists == false) SetUserLocale(L"en-US");
		else SetUserLocale(oldUILang);
	}
	else {
		switch (settings["ui_lang"].get<int>()) {
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
		default: SetUserLocale(L"en-US");
		}
	}

	// THREAD 생성
	DWORD threadId;
	DWORD bgthreadId;

	// 스레드 생성
	g_MainUIThreadID = GetCurrentThreadId(); // 메인 UI 스레드 ID 저장

	if (!bActThread) {
		hThread = CreateThread(
			NULL, // 기본 보안 속성
			0, // 기본 스택 크기
			ThreadProc, // 스레드 함수
			NULL, // 스레드 함수에 전달될 파라미터
			0, // 생성 옵션: 스레드 즉시 실행
			&threadId); // 스레드 ID

		if (hThread == NULL) {
			std::cerr << "Failed to create thread." << std::endl;
			return 1;
		}
		bActThread = TRUE;
	}

	if (!bBgThread) {
		hBgThread = CreateThread(
			NULL, // 기본 보안 속성
			0, // 기본 스택 크기
			BgProc, // 스레드 함수
			NULL, // 스레드 함수에 전달될 파라미터
			0, // 생성 옵션: 스레드 즉시 실행
			&bgthreadId); // 스레드 ID

		if (hBgThread == NULL) {
			std::cerr << "Failed to create Bg thread." << std::endl;
			return 1;
		}
		bBgThread = TRUE;
	}

	if (GetCurrentThreadId() == g_MainUIThreadID) {
		MyRegisterClass(hInstance);

		// 애플리케이션 초기화를 수행합니다:
		if (!InitInstance(hInstance, nCmdShow))
		{
			return FALSE;
		}

		HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REALTRANS));
		RunTimer();

		MSG msg;

		// 기본 메시지 루프입니다:
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return (int)msg.wParam;
	}

	return 0;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REALTRANS));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = APP_TITLE;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_REALTRANS));
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // 배경을 검은색으로 설정


	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	LoadLibrary(TEXT("Msftedit.dll"));
	LoadLibrary(TEXT("ole32.dll"));
	LoadLibrary(TEXT("uuid.dll"));
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE, // 확장 스타일
		APP_TITLE,      // 윈도우 클래스 이름
		APP_TITLE_VERSION, // 윈도우 타이틀 (이 경우에는 보이지 않음)
		WS_OVERLAPPEDWINDOW & ~WS_CAPTION, // 기본 스타일에서 WS_CAPTION을 제거
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 400, // 위치 및 크기
		NULL, NULL, hInstance, NULL
	);

	//HWND hWnd = CreateWindowEx(
	//	WS_EX_TOPMOST | WS_EX_LAYERED, //WS_EX_LAYERED,                // 투명도 설정을 위한 확장 스타일
	//	L"RoundWindow",               // 클래스 이름
	//	NULL,                         // 타이틀바 없음
	//	WS_POPUP,                     // 타이틀바와 테두리 제거된 윈도우
	//	CW_USEDEFAULT, CW_USEDEFAULT, 1280, 400, // 위치 및 크기
	//	//hParent, NULL, hInstance, NULL);
	//	NULL, NULL, hInstance, NULL);

	if (!hWnd) return FALSE;

	// 요약창
	DlgSum = new CDlgSummary(hInstance);
	DlgSum->Create(hWnd);
	DlgSum->Hide();

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


//GdiplusStartupInput gdiplusStartupInput;
//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (GetCurrentThreadId() != g_MainUIThreadID) {
		return 0;
	}

	static BOOL isMouseInside = FALSE; // 마우스가 창 내부에 있는지 여부

	// 번역창
	//if (DlgTrans != NULL) {
	//	int act = 0;
	//	switch(message) {
	//	case 273: // WM_COMMAND
	//	case 32: // MOUSEMOVE
	//	case 132: // WM_NCHITTEST
	//	case 512: // WM_MOUSEMOVE
	//	case 70: // WM_WINDOWPOSCHANGING
	//	case 71: // WM_WINDOWPOSCHANGED
	//	case 160: // WM_NCACTIVATE :  비-클라이언트 영역에는 윈도우의 테두리, 제목 표시줄, 메뉴, 그리고 시스템 버튼(최소화, 최대화, 닫기 등)이 포함
	//	case 20: // WM_SETCURSOR
	//	case 297: // WM_NCMOUSEMOVE
	//	case 295: // WM_NCMOUSELEAVE
	//	//case 296: // WM_NCLBUTTONDOWN
	//	case 307: // WM_NCLBUTTONUP
	//	case 674: // WM_NCMOUSEHOVER
	//	case 134: // WM_NCACTIVATE
	//	case 528: // WM_MOUSEHOVER
	//	case 641: // WM_NCMOUSELEAVE
	//	case 642: // WM_NCMOUSEHOVER
	//	case 534: // WM_MOUSEHOVER
	//	case 36: // WM_GETMINMAXINFO
	//	case 3: // WM_MOVE
	//		break;
	//	default:
	//		act = 1;
	//	}
	//	if (act == 1) {
	//		char tmp[100];
	//		sprintf_s(tmp, "isVisible=%d, %d", DlgTrans->bVisible, message);
	//		AppendTextToRichEdit(hwndTextBox, (WCHAR*)Utf8ToWideString(tmp).c_str());
	//	}

	//}
	if( DlgTrans!=NULL && DlgTrans->bVisible == TRUE ) DlgTrans->WndProc(hWnd, message, wParam, lParam);

	//long style;
	switch (message)
	{
	case WM_CREATE:
	{
		// 윈도우 투명도 설정
		// 타이틀바를 사용하지 않으면 전체 테두리도 사용하지 않게된다.
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWnd, 0, (255 * 80) / 100, LWA_ALPHA);

		// 타이틀 아이콘의 초기 위치 설정
		iconRect = { 500, 3, 34, 34 }; // 실제 프로젝트에서는 타이틀바의 위치와 크기를 계산하여 설정

		// RichEdit 컨트롤 생성
		hwndTextBox = CreateWindowEx(0, L"RichEdit50W", TEXT(""),
									WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
									33, 0, 300, 200, hWnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		// 폰트 생성
		hFont = CreateFont(30, // -MulDiv(20, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72), // 높이
			15,                          // 너비
			0,                          // 회전 각도
			0,                          // 기울기 각도
			FW_BOLD, // FW_NORMAL,                  // 폰트 굵기
			FALSE,                      // 이탤릭
			FALSE,                      // 밑줄
			FALSE,                      // 취소선
			ANSI_CHARSET, //HANGUL_CHARSET, //ANSI_CHARSET,               // 문자 집합
			OUT_TT_PRECIS, //OUT_DEFAULT_PRECIS,         // 출력 정밀도
			CLIP_DEFAULT_PRECIS,        // 클리핑 정밀도
			DEFAULT_QUALITY,            // 출력 품질
			DEFAULT_PITCH | FF_ROMAN, //| FF_DONTCARE, //DEFAULT_PITCH | FF_SWISS,   // 피치와 패밀리
			TEXT("Arial"));             // 폰트 이름

		SendMessage(hwndTextBox, WM_SETFONT, (WPARAM)hFont, TRUE);

		// 배경색 변경
		SendMessage(hwndTextBox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));

		// 메뉴 생성 : GDI만 사용 ( Image 관련 기능 안됨 )
		lmenu = new CMenusDlg();

		// 창을 항상 맨 위에 위치하게 설정
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// 0.5초 간격으로 타이머 설정
		SetTimer(hWnd, TIMER_ID, 500, NULL);
	}
	break;
	case WM_WINDOWPOSCHANGED:
	{
		WINDOWPOS* pPos = (WINDOWPOS*)lParam;
		// 창이 최상위인지 확인
		LONG style = GetWindowLong(hWnd, GWL_EXSTYLE);
		if (!(style & WS_EX_TOPMOST)) {
			// 최상위가 아니면 최상위로 설정
			if (!(pPos->flags & SWP_NOZORDER)) {
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				return 0; // 메시지 처리 종료
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		if (DlgTrans == NULL || DlgTrans->bVisible == FALSE) {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			Graphics graphics(hdc);
			if (bStopMenu == FALSE) {
				Image image(L"lmenu_normal.png");
				graphics.DrawImage(&image, 0, 0, image.GetWidth(), image.GetHeight());
			}
			else {
				Image image(L"lmenu_over.png");
				graphics.DrawImage(&image, 0, 0, image.GetWidth(), image.GetHeight());
			}

			EndPaint(hWnd, &ps);
		}
	}
	break;

	case WM_LBUTTONDOWN:
	{
		POINTS pt = MAKEPOINTS(lParam);

		// 특정 영역 예: (100, 100) ~ (200, 200)
		if (pt.x > 0 && pt.x < 34 && pt.y > 0 && pt.y < 44) {
			// 환경설정
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETUP), hWnd, DialogProc);
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 44 && pt.y < 81) {
			// 요약
			DlgSum->Create(hWnd);
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 81 && pt.y < 113) {
			// 실행/중지
			bStopMenu = !bStopMenu;
			RECT rect;
			rect.left = 0;   // 시작 X 좌표
			rect.top = 0;    // 시작 Y 좌표
			rect.right = 34; // 끝 X 좌표
			rect.bottom = 226; // 끝 Y 좌표
			InvalidateRect(hWnd, &rect, FALSE); // 지정된 영역만 무효화합니다. 배경은 지우지 않습니다.

			// json 파일로 저장
			if (bStopMenu == TRUE) {
				MakeChildCmd("xx", "xx");
			}
			else {
				std::string tgt_lang;
				if (settings["ck_pctrans"] == true) tgt_lang = settings["cb_pctrans_lang"];
				else tgt_lang = "xx";
				MakeChildCmd(settings["cb_voice_lang"], tgt_lang);
			}
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 113 && pt.y < 140) {
			ShellExecuteA(0, "open", "chrome.exe", "https://www.youtube.com/@markets/streams", NULL, SW_SHOWNORMAL);
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 140 && pt.y < 168) {
			ShellExecuteA(0, "open", "chrome.exe", "https://www.financialjuice.com/home", NULL, SW_SHOWNORMAL);
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 168 && pt.y < 197) {
			ShellExecuteA(0, "open", "chrome.exe", "https://edition.cnn.com/markets/fear-and-greed", NULL, SW_SHOWNORMAL);
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 197 && pt.y < 226) {
			// 번역창 전환
			if (DlgTrans == NULL) {
				DlgTrans = new CDlgTrans(hInst);
				DlgTrans->Create(hWnd);
			}

			if (DlgTrans->bVisible==TRUE) {
				DlgTrans->Hide();
				ShowWindow(hwndTextBox, SW_SHOW);
			}
			else {
				ShowWindow(hwndTextBox, SW_HIDE);
				DlgTrans->Show();
			}
		}
		else if (pt.x > 0 && pt.x < 34 && pt.y > 234 && pt.y < 266) {
			//ShellExecuteA(0, "open", "chrome.exe", "https://www.cnbc.com/world/?region=world", NULL, SW_SHOWNORMAL);
			// 정보
			DlgInfo = new CDlgInfo(hInst);
			DlgInfo->Create(hWnd);
		}
	}
	break;
	case WM_SIZE:
	{
		int width = LOWORD(lParam); // 새로운 윈도우의 너비
		int height = HIWORD(lParam); // 새로운 윈도우의 높이

		// 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
		MoveWindow(hwndTextBox, 43, 10, width - 43, height - 20, TRUE);
	}
	break;
	case WM_TIMER: {
		if (wParam == TIMER_ID) {
			// 마우스 위치 얻기
			POINT pt;
			GetCursorPos(&pt);

			// 마우스가 창 내부에 있는지 확인
			RECT rc;
			GetWindowRect(hWnd, &rc);
			BOOL isInside = PtInRect(&rc, pt);

			// 상태가 변경되었을 때만 처리
			if (isMouseInside != isInside) {
				isMouseInside = isInside;

				if (isMouseInside) {
					// 마우스가 창 내부로 들어옴
					// 타이틀바 및 테두리를 추가
					SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
					SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
						SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
				}
				else {
					// 마우스가 창 외부로 나감
					SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_LAYERED); // 확장 스타일을 모두 제거
					SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
					SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
						//SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
						SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
				}
			}
		}
	} break;
	case WM_DESTROY:
		// Kill process if it is still running. 
		TerminateProcess(piProcInfo.hProcess, 0);
		// 스레드 핸들 닫기
		CloseHandle(hThread);
		// 프로세스 및 핸들 정리
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		CloseHandle(hChildStdOutRead);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

