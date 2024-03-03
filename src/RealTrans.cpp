// RealTrans.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "RealTrans.h"
#include <CommCtrl.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <atlstr.h>
#include <stdlib.h>
#include <Richedit.h>
#include <shellapi.h>
#include <WinUser.h>
#include <tlhelp32.h>
#include "SoundDev.h"
#include "CMenusDlg.h"
#include "CSettings.h"
#include "resource.h"
#include "json.hpp"
#include "CDlgSummary.h"
#include "StrConvert.h"
#include "deepl_api.h"

// nlohmann 라이브러리의 네임스페이스 사용
using json = nlohmann::json;

#define MAX_LOADSTRING 100

// 전역 변수로 타이틀바 아이콘에 대한 식별자와 위치를 정의
#define IDI_SETTINGS 132
RECT iconRect;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

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
CMenusDlg *lmenu;

// Thread
HANDLE hThread;
HANDLE hBgThread;
// 글로벌 변수로 메인 UI 스레드 ID 저장
DWORD g_MainUIThreadID;
DWORD g_WorkThreadID;

char addText[4096] = "";
bool addFlag = false;
std::string strEng = "";

// 환경설정
BOOL isRefreshEnv = false;
json settings;

// 요약창
CDlgSummary *DlgSum;
time_t nSummaryTime = 0;
BOOL runSummary = false;
extern BOOL addSummaryFlag;

void DebugHandle(HWND value)
{
    WCHAR buf[200];
    wsprintf(buf, L"Handle: %d", value);
    SendMessage(hwndTextBox, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
}

void DebugText(const WCHAR *text)
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

    //cf.dwMask = CFM_COLOR; // 색상 변경을 나타냅니다.
    //cf.crTextColor = color; // 원하는 색상을 RGB 값으로 설정합니다.
    cf.crTextColor = HexToCOLORREF(settings["ed_oldfont_color"]); // 원하는 색상을 RGB 값으로 설정합니다.

    // 폰트 색상 변경을 적용합니다.
    //SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

    //void SetRichEditTextColor2(HWND hRichEdit, COLORREF color) {
    //	CHARFORMAT2 cf;
    //	ZeroMemory(&cf, sizeof(cf));
    //	cf.cbSize = sizeof(cf);
    //	cf.dwMask = CFM_SIZE | CFM_COLOR;
    //	cf.yHeight = settings["cb_sumfont_size"] * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환
    //	//SendMessage(hRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    //
    //	//cf.dwMask = CFM_COLOR; // 색상 변경을 나타냅니다.
    //	cf.crTextColor = color; // 원하는 색상을 RGB 값으로 설정합니다.
    //
    //	// 폰트 색상 변경을 적용합니다.
    //	SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    //}
    // 
    // 
// 최신 입력글 색상과 사이즈를 변경하는 함수
void SetRichEditSelColor(HWND hRichEdit) {
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_SIZE | CFM_COLOR; // 폰트 크기와 색상 변경을 나타냅니다.
    cf.yHeight = settings["cb_font_size"] * 20; // 포인트 단위의 폰트 크기를 트웬티엣으로 변환

    //cf.dwMask = CFM_COLOR; 
    //cf.crTextColor = color; // 원하는 색상을 RGB 값으로 설정합니다.
    cf.crTextColor = HexToCOLORREF(settings["ed_font_color"]); // 원하는 색상을 RGB 값으로 설정합니다.

    // 폰트 색상 변경을 적용합니다.
    SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// hRichEdit는 RichEdit 컨트롤의 핸들입니다.
// textToAdd는 추가하고자 하는 텍스트입니다.
void AppendTextToRichEdit(HWND hRichEdit, WCHAR *textToAdd) {
    // 현재 선택을 문서의 끝으로 설정하여 텍스트 추가 위치를 문서 끝으로 합니다.
    // 이는 텍스트 선택이 없을 경우 커서 위치를 문서 끝으로 이동시키는 효과가 있습니다.
    //HexToCOLORREF(settings["ed_font_color"]);

    //SetRichEditTextColor(hRichEdit, RGB(140, 140, 140));
    SetRichEditTextColor(hRichEdit);
    SendMessage(hRichEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

    // 커서 위치(현재 선택의 끝)를 얻습니다.
    DWORD start = 0, end = 0;
    SendMessage(hRichEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
   
    // EM_REPLACESEL 메시지를 사용하여 현재 선택(또는 커서 위치)에 텍스트를 추가합니다.
    // WPARAM의 TRUE는 텍스트 추가 후 Undo 가능하도록 합니다.
    // EM_SETTEXTEX 구조체를 준비합니다.
    //SETTEXTEX st;
    //st.flags = ST_DEFAULT;
    //st.codepage = 1200; // UTF-16 코드 페이지
    SendMessageW(hRichEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)textToAdd);

    // 선택해서 색 변경
    int textLength = GetWindowTextLength(hRichEdit);
    // 커서를 문서의 끝으로 이동해서 선택.
    SetFocus(hRichEdit);
    SendMessage(hRichEdit, EM_SETSEL, end, -1);
    SetRichEditSelColor(hRichEdit);
    //SetRichEditSelColor(hRichEdit, RGB(200,200,255));

    // 선택을 취소하고 싶다면 다음과 같이 합니다.
    // 이는 커서를 문서의 끝으로 이동시킨 후 선택 영역이 없도록 합니다.
    SendMessage(hRichEdit, EM_SETSEL, -1, 0);
    //ScrollToBottomSel(hRichEdit);
}

void CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime);

void RunTimer() {
    UINT_PTR timerId = 1; // 타이머 ID
    UINT elapse = 40; // ms

    // 타이머 설정
    if (SetTimer(NULL, timerId, elapse, (TIMERPROC)TimerProc) == 0) {
        std::cerr << "Failed to set timer" << std::endl;
        return;
    }
}

void CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime)
{
    WCHAR strT[4096];

    // 설정 변경시 
    if (isRefreshEnv) {
        // python work 종료 : pipe 연결된 프로세스 종료
        // pipe 재시작
    }

    if (addFlag && strlen(addText)>0 ) {
        if (strlen(addText) <= 2 && addText[0] == 13 && addText[1] == 10) return;

        if (addText[0] == '-') {
            if (settings["ck_orgin_subtext"] == true) {
                CharToWChar(strT, addText);
                AppendTextToRichEdit(hwndTextBox, (WCHAR*)L"\r\n");
				AppendTextToRichEdit(hwndTextBox, strT);
			}
            if (settings["ck_apitrans"] == true) {
                // API 번역
                if (settings["cb_trans_api"] == 0) // DeepL
                {
                    std::string strOut;
                    std::string strIn;
                    std::string lang;
                    strIn = &addText[1];
                    if (settings["cb_apitrans_lang"]==0) lang = "KO";
                    else lang = "EN";

                    RequestDeepL(strIn, strOut, lang, settings["ed_trans_api_key"]);
                    //CharToWChar(strT, strOut.c_str());
                    StringToWChar(strOut, strT);
                    AppendTextToRichEdit(hwndTextBox, (WCHAR*)L"\r\n");
                    AppendTextToRichEdit(hwndTextBox, strT);
                    //AppendTextToRichEdit(hwndTextBox, (WCHAR*)L"TEST");
                }
            }
        }
        else {
            if (settings["ck_pctrans"] == true) {
				CharToWChar(strT, addText);
				AppendTextToRichEdit(hwndTextBox, (WCHAR*)L"\r\n ");
				AppendTextToRichEdit(hwndTextBox, strT);
			}
        }
        addFlag = false;
    }

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
    if (runSummary == true) {
        DlgSum->BgSummary();
        //DlgSum->Trace(L"TEST");
        runSummary = false;
    }
}

// 작업 Thread : Python 프로그램과 pipe로 번역된 결과를 받아서 내부에 적재
DWORD WINAPI ThreadProc(LPVOID lpParam)
{
     // sjheo 동기 읽기 시작
     DWORD dwRead;
     CHAR chBuf[4096];

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

     // 기본 사운드 장치 검색해서 Python 실행인자로 넘겨줌
     WCHAR strActSound[200];
     if (GetActiveSound(strActSound) == TRUE) {
         SendMessage(hwndTextBox, WM_SETTEXT, 0, (LPARAM)strActSound);
     }

     // 인자 설정
     // 영어 (en), 스페인어(es), 프랑스어(fr), 독일어(de), 중국어(zh), 일본어(ja), 한국어(ko), 이탈리아어(it), 포르투갈어(pt), 러시아어(ru), 아랍어(ar), 힌디어(hi)
     // -v en -t ko : voice(영어), Trans(한국어)
     WCHAR strLang[100];
     switch( (int)settings["cb_voice_lang"]) {
         case 0: // 영어
             swprintf(strLang, 100, L"-v en ");
             break;
	 }
     if( settings["ck_pctrans"] == true )
         switch ( (int)settings["cb_pctrans_lang"]) {
		     case 0: // 한국어
		        wcscat_s(strLang, L"-t ko ");
                break;
             case 1: // 영어
                 wcscat_s(strLang, L"-t en ");
			    break;
         }
     else wcscat_s(strLang, L"-t xx ");

     // 외부 프로그램 실행 : python 번역 프로그램 ( runTransWin.py)
     TCHAR cmd[300]; // 실행 명령
#ifndef NDEBUG
     swprintf(cmd, 300, L"python.exe D:\\work\\trans\\runTransWin.py -d \"%s\" %s", strActSound, strLang);
#else
     swprintf(cmd, 300, L"runTransWin.exe -d \"%s\" %s", strActSound, strLang);
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
         & siStartInfo,     // STARTUPINFO 구조체
         & piProcInfo       // PROCESS_INFORMATION 구조체
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
         if ( dwRead>0 ) {
             chBuf[dwRead] = '\0';
             strcpy_s(addText, chBuf);
             addFlag=true;
         }
         else chBuf[0] = '\0';

         // 원문 영어 자막
         if ( strlen(chBuf) > 1 && chBuf[0] == '-'  ) {
             strEng += &chBuf[1];
             strEng += "\n";
         }


         Sleep(60);
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

    // THREAD 생성
    DWORD threadId;
    DWORD bgthreadId;

    // 스레드 생성
    g_MainUIThreadID = GetCurrentThreadId(); // 메인 UI 스레드 ID 저장

    ///*
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
    //*/

    if (GetCurrentThreadId() == g_MainUIThreadID) {
        // 전역 문자열을 초기화합니다.
        LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_REALTRANS, szWindowClass, MAX_LOADSTRING);
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
            //if (!IsDialogMessage(msg.hwnd, &msg)) {
                if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            //}
        }

        return (int)msg.wParam;
    }
    // END: 코드 종료

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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REALTRANS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = 0;
    //wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_REALTRANS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
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
   LoadLibrary(TEXT("Riched20.dll"));
   LoadLibrary(TEXT("ole32.dll"));
   LoadLibrary(TEXT("uuid.dll"));
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowEx(
       WS_EX_CLIENTEDGE, // 확장 스타일
       szWindowClass,      // 윈도우 클래스 이름
       szTitle, // 윈도우 타이틀 (이 경우에는 보이지 않음)
       WS_OVERLAPPEDWINDOW & ~WS_CAPTION, // 기본 스타일에서 WS_CAPTION을 제거
       CW_USEDEFAULT, CW_USEDEFAULT, 1280, 400, // 위치 및 크기
       NULL, NULL, hInstance, NULL
   );

   if (!hWnd) return FALSE;
   
   /*
   // WS_CAPTION 제거가 되지 않는다.
   // 윈도우 핸들이 hWnd인 윈도우의 타이틀바를 제거
   LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
   lStyle &= ~(WS_CAPTION); // WS_CAPTION 스타일 비트를 클리어
   SetWindowLong(hWnd, GWL_STYLE, lStyle);

   // 스타일 변경 후, 윈도우를 다시 그려야 변경 사항이 적용됩니다.
   SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
   */

   // 요약창
   DlgSum=new CDlgSummary(hInstance);
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
        //return DefWindowProc(hWnd, message, wParam, lParam);
        return 0;
    }

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
            hwndTextBox = CreateWindowEx(0, RICHEDIT_CLASS, TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                33, 0, 300, 200, hWnd, (HMENU)1, GetModuleHandle(NULL), NULL);

            // 창을 항상 맨 위에 위치하게 설정
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            // 폰트 생성
            hFont = CreateFont(30, // -MulDiv(20, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72), // 높이
                15,                          // 너비
                0,                          // 회전 각도
                0,                          // 기울기 각도
                FW_BOLD, // FW_NORMAL,                  // 폰트 굵기
                FALSE,                      // 이탤릭
                FALSE,                      // 밑줄
                FALSE,                      // 취소선
                HANGUL_CHARSET, //ANSI_CHARSET,               // 문자 집합
                OUT_TT_PRECIS, //OUT_DEFAULT_PRECIS,         // 출력 정밀도
                CLIP_DEFAULT_PRECIS,        // 클리핑 정밀도
                DEFAULT_QUALITY,            // 출력 품질
                DEFAULT_PITCH | FF_ROMAN, //| FF_DONTCARE, //DEFAULT_PITCH | FF_SWISS,   // 피치와 패밀리
                TEXT("D2Coding"));             // 폰트 이름

            // 폰트 적용
            SendMessage(hwndTextBox, WM_SETFONT, (WPARAM)hFont, TRUE);

            // 배경색 변경
            SendMessage(hwndTextBox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0)); 

            // 메뉴 생성 : GDI만 사용 ( Image 관련 기능 안됨 )
            lmenu = new CMenusDlg();
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
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
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            //hdc = GetWindowDC(hWnd);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            // LEFT Menu 생성
            Graphics graphics(hdc);
            Image image(L"lmenu_normal.png");
            graphics.DrawImage(&image, 0, 10, image.GetWidth(), image.GetHeight());

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_LBUTTONDOWN:
        {
            POINTS pt = MAKEPOINTS(lParam);

            // 특정 영역 예: (100, 100) ~ (200, 200)
            if (pt.x > 0 && pt.x < 34 && pt.y > 10 && pt.y < 44) {
                // 다이얼로그 생성
                //DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_YOUR_DIALOG), hWnd, YourDialogProc, 0);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETUP), hWnd, DialogProc);
            }
            else if (pt.x > 0 && pt.x < 34 && pt.y > 44 && pt.y < 78) {
				// 메뉴 생성
                DlgSum->Create(hWnd);
			}
            else if (pt.x > 0 && pt.x < 34 && pt.y > 138 && pt.y < 170) {
                ShellExecuteA(0, "open", "chrome.exe", "https://www.youtube.com/@markets/streams", NULL, SW_SHOWNORMAL);
            }
            else if (pt.x > 0 && pt.x < 34 && pt.y > 170 && pt.y < 202) {
                ShellExecuteA(0, "open", "chrome.exe", "https://www.financialjuice.com/home", NULL, SW_SHOWNORMAL);
            }
            else if (pt.x > 0 && pt.x < 34 && pt.y > 202 && pt.y < 234) {
                ShellExecuteA(0, "open", "chrome.exe", "https://edition.cnn.com/markets/fear-and-greed", NULL, SW_SHOWNORMAL);
            }
            else if (pt.x > 0 && pt.x < 34 && pt.y > 234 && pt.y < 266) {
                ShellExecuteA(0, "open", "chrome.exe", "https://www.cnbc.com/world/?region=world", NULL, SW_SHOWNORMAL);
            }
        }
        break;
    case WM_SIZE:
        {
            int width = LOWORD(lParam); // 새로운 윈도우의 너비
            int height = HIWORD(lParam); // 새로운 윈도우의 높이

            // 텍스트박스의 크기와 위치를 윈도우의 크기에 맞추어 조정
            // MoveWindow(hwndTextBox, 33, 10, width - 20, height - 20, TRUE);
            MoveWindow(hwndTextBox, 43, 10, width - 43, height - 20, TRUE);
            //MoveWindow(hwndTextBox, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        break;
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
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

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

