#include "BgJob.h"
#include "CDlgSummary.h"
#include "CDlgTrans.h"
#include "StrConvert.h"
#include "deepl_api.h"
#include "json.hpp"
#include "Util.h"

using json = nlohmann::json;

// 싱글톤 인스턴스 초기화
BgJob* BgJob::s_instance = nullptr;
extern json settings;

BgJob::BgJob()
    : m_hThread(NULL)
    , m_hBgThread(NULL)
    , m_bActThread(false)
    , m_bBgThread(false)
    , m_isExit(false)
    , m_WorkThreadID(0)
    , m_addFlag(false)
    , m_nSummaryTime(0)
    , m_runSummary(false)
    , m_pDlgSum(nullptr)
    , m_pDlgTrans(nullptr)
    , m_hChildStdOutRead(NULL)
    , m_hChildStdOutWrite(NULL)
    , m_pViewer(nullptr)
{
    ZeroMemory(&m_piProcInfo, sizeof(PROCESS_INFORMATION));
}

BgJob::~BgJob() {
    Cleanup();
}

// 싱글톤 접근자
BgJob* BgJob::GetInstance() {
    if (s_instance == nullptr) {
        s_instance = new BgJob();
    }
    return s_instance;
}

bool BgJob::Initialize() {
    // 파이썬 프로세스 종료
    //TerminatePythonProcess();

    return true;
}

void BgJob::RunTimer() {
    UINT_PTR timerId = 1; // 타이머 ID
    UINT elapse = 20; // ms

    // 타이머 설정
    if (SetTimer(NULL, timerId, elapse, (TIMERPROC)TimerProc) == 0) {
        std::cerr << "타이머 설정 실패" << std::endl;
        return;
    }
}

void BgJob::Cleanup() {
    // 종료 플래그 설정
    m_isExit = true;

    // 스레드 핸들 닫기
    if (m_hThread != NULL) {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    if (m_hBgThread != NULL) {
        CloseHandle(m_hBgThread);
        m_hBgThread = NULL;
    }

    // 파이썬 프로세스 종료
    TerminateProcess(m_piProcInfo.hProcess, 0);

    // 프로세스 및 핸들 정리
    CloseHandle(m_piProcInfo.hProcess);
    CloseHandle(m_piProcInfo.hThread);
    CloseHandle(m_hChildStdOutRead);
    CloseHandle(m_hChildStdOutWrite);
}

void BgJob::SetSummaryDialog(CDlgSummary* pDlgSum) {
    m_pDlgSum = pDlgSum;
}

void BgJob::SetTransDialog(CDlgTrans* pDlgTrans) {
    m_pDlgTrans = pDlgTrans;
}

void BgJob::SetViewer(NemoViewer* pViewer) {
    m_pViewer = pViewer;
}

bool BgJob::AddTextToBuffer(const std::string& text) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_addText += text;
    m_addFlag = true;
    return true;
}

std::string BgJob::GetSrcText() {
    std::lock_guard<std::mutex> lock(m_mtx);
    std::string tmp = m_strSrc;
    m_strSrc.clear();
    return tmp;
}

int BgJob::GetSrcSize() {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_strSrc.size();
}

void BgJob::ResetSummaryTime() {
    m_nSummaryTime = 0;
}

void BgJob::RequestExit() {
    m_isExit = true;
}

bool BgJob::IsExitRequested() const {
    return m_isExit;
}

// static 타이머 프로시저
void CALLBACK BgJob::TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime)
{
    BgJob* pInstance = BgJob::GetInstance();
    if (pInstance->IsExitRequested()) return;
    WCHAR strT[4096];

    if (pInstance->m_addFlag && pInstance->m_addText.size() > 0) {
        if (pInstance->m_addText.size() <= 2) {
            std::lock_guard<std::mutex> lock(pInstance->m_mtx);
            pInstance->m_addText.clear();
            pInstance->m_addFlag = false;
            return;
        }

        std::wstring convertBuf;
        std::string readBuf;
        {
            std::lock_guard<std::mutex> lock(pInstance->m_mtx);
            readBuf = pInstance->m_addText;
            pInstance->m_addText.clear();
            pInstance->m_addFlag = false;
        }

        readBuf = Trim(readBuf);
        TRACE("%s\n", readBuf.c_str());
        if (readBuf[0] == '-') {
            TRACE(L"TYPE[-]\n");
            if (settings["ck_orgin_subtext"] == true) {
                convertBuf = Utf8ToWideString(readBuf);
                if (convertBuf.size() > 0) {
                    if (pInstance->m_pViewer) {
                        pInstance->m_pViewer->AddText(convertBuf.c_str());
                    }
                }
            }
            if (settings["ck_apitrans"] == true) {
                // API 번역
                if (settings["cb_trans_api"] == 0) // DeepL
                {
                    std::string strOut;
                    RequestDeepL(&readBuf[1], strOut, settings["cb_apitrans_lang"],
                        settings["ed_trans_api_key"], 1);
                    std::wstring strChg = StringToWCHAR(strOut);
                    if (pInstance->m_pViewer) {
                        pInstance->m_pViewer->AddText(strChg.c_str());
                    }
                }
                else if (settings["cb_trans_api"] == 1) // DeepL Pro
                {
                    std::string strOut;
                    RequestDeepL(&readBuf[1], strOut, settings["cb_apitrans_lang"],
                        settings["ed_trans_api_key"], 2);
                    std::wstring strChg = StringToWCHAR(strOut);
                    if (pInstance->m_pViewer) {
                        pInstance->m_pViewer->AddText(strChg.c_str());
                    }

                }
            }
        }
        // 번역 진행도
        else if (readBuf[0] == '[') {
            TRACE(L"TYPE[[]\n");
            if (pInstance->m_pDlgTrans != NULL && pInstance->m_pDlgTrans->isTrans == TRUE) {
                // ']'를 찾아서 그 앞까지 읽어서 Percent를 만든다.
                std::string strPercent = readBuf.substr(1, readBuf.find(']'));
                int nPercent = atoi(strPercent.c_str());
                pInstance->m_pDlgTrans->transPercent = nPercent;
            }
        }
        else {
            TRACE(L"TYPE[]\n");
            if (settings["ck_pctrans"] == true || readBuf[0] == '#') {
                convertBuf = Utf8ToWideString(readBuf);
                if (convertBuf.size() > 0) {
                    if (pInstance->m_pViewer) {
                        pInstance->m_pViewer->AddText(convertBuf.c_str());
                    }
                }
            }
        }

        convertBuf.clear();
    }

    // 요약창에 추가할 것이 있으면 처리
    //if (pInstance->m_pDlgSum && pInstance->m_pDlgSum->addSummaryFlag == true)
    //    pInstance->m_pDlgSum->AddRichText();
}

void BgJob::RunJobs() {
    if (m_isExit) return;

    // 자동요약 처리
    if (!settings.is_null() && settings["ck_summary"].get<bool>() == true) {
        time_t now = time(0);
        // 초기 요약시간 설정
        if (m_nSummaryTime == 0) m_nSummaryTime = now + settings["cb_summary_min"] * 60;

        if (m_nSummaryTime < now) {
            // 요약창의 요약버튼 클릭이벤트 발생
            m_nSummaryTime = now + settings["cb_summary_min"] * 60;
            m_runSummary = true;
        }
    }

    // 요약 실행
    if (m_runSummary == true && m_pDlgSum && m_strSrc.size()>0) {
        m_pDlgSum->BgSummary(m_strSrc);
        m_strSrc.clear();
        m_runSummary = false;
    }

    // 번역파일 처리: translate_out.txt 파일이 있으면 번역창에 표시하고 삭제한다.
    if (m_pDlgTrans != NULL && m_pDlgTrans->bVisible) {
        m_pDlgTrans->CheckTransFile();
        m_pDlgTrans->ProgressBar(m_pDlgTrans->transPercent);
    }
}

bool BgJob::StartThreads() {
    // 스레드 ID
    DWORD threadId;
    DWORD bgthreadId;

    // 작업 스레드 생성
    if (!m_bActThread) {
        m_hThread = CreateThread(
            NULL,         // 기본 보안 속성
            0,            // 기본 스택 크기
            ThreadProc,   // 스레드 함수
            this,         // 스레드 함수에 전달될 파라미터 (this 포인터)
            0,            // 생성 옵션: 스레드 즉시 실행
            &threadId);   // 스레드 ID

        if (m_hThread == NULL) {
            std::cerr << "스레드 생성 실패." << std::endl;
            return false;
        }
        m_bActThread = true;
    }

    // 백그라운드 스레드 생성
    if (!m_bBgThread) {
        m_hBgThread = CreateThread(
            NULL,         // 기본 보안 속성
            0,            // 기본 스택 크기
            BgProc,       // 스레드 함수
            this,         // 스레드 함수에 전달될 파라미터 (this 포인터)
            0,            // 생성 옵션: 스레드 즉시 실행
            &bgthreadId); // 스레드 ID

        if (m_hBgThread == NULL) {
            std::cerr << "백그라운드 스레드 생성 실패." << std::endl;
            return false;
        }
        m_bBgThread = true;
    }

    return true;
}

// 스레드 프로시저: Python 프로그램과 pipe로 번역된 결과를 받아서 내부에 저장
DWORD WINAPI BgJob::ThreadProc(LPVOID lpParam)
{
    BgJob* pThis = static_cast<BgJob*>(lpParam);

    // 동기 읽기 시작
    DWORD dwRead;
    CHAR chBuf[4097];
    std::string strReadBuf;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // 스레드 ID 저장
    pThis->m_WorkThreadID = GetCurrentThreadId();

    // 표준 출력 파이프 생성
    if (!CreatePipe(&pThis->m_hChildStdOutRead, &pThis->m_hChildStdOutWrite, &saAttr, 0)) {
        std::cerr << "파이프 생성 실패\n";
        return 1;
    }

    STARTUPINFO siStartInfo;
    ZeroMemory(&pThis->m_piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = pThis->m_hChildStdOutWrite;
    siStartInfo.hStdOutput = pThis->m_hChildStdOutWrite;
    siStartInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    siStartInfo.wShowWindow = SW_HIDE;

    std::string strProc;
    std::wstring wstrProc;

    if (!settings.is_null()) {
        strProc = "-d " + settings.value("input_dev", "") + " -p \"" +
            settings.value("proc", "cuda float16") + "\"";
        wstrProc = StringToWCHAR(strProc);

        std::string strLang;
        std::wstring wstrLang;

        if (settings["ck_pctrans"] == true)
            strLang += "-s " + settings.value("cb_voice_lang", "") +
            " -t " + settings.value("cb_pctrans_lang", "");
        else
            strLang += "-s " + settings.value("cb_voice_lang", "") + " -t xx";

        // 언어 모델 크기 설정
        strLang += " -m " + settings.value("model_size", "");
        wstrLang = StringToWCHAR(strLang);

        // 외부 프로그램 실행: python 번역 프로그램 (runTransWin.py)
        TCHAR cmd[300]; // 실행 명령
#ifndef NDEBUG // 디버그 모드
        swprintf(cmd, 300, L"python.exe D:\\work\\trans\\runTransWin.py %s %s",
            wstrProc.c_str(), wstrLang.c_str());
#else
        swprintf(cmd, 300, L"python.exe runTransWin.py %s %s",
            wstrProc.c_str(), wstrLang.c_str());
#endif

        if (!CreateProcess(
            NULL,             // 실행 파일 경로
            cmd,              // 명령줄 인수 (없으면 NULL)
            NULL,             // 프로세스 핸들 상속 옵션
            NULL,             // 스레드 핸들 상속 옵션
            TRUE,             // 핸들 상속 옵션
            CREATE_NEW_CONSOLE, // 새 콘솔 창 생성 옵션
            NULL,              // 환경 변수 (없으면 NULL)
            NULL,              // 현재 디렉토리 (없으면 NULL)
            &siStartInfo,      // STARTUPINFO 구조체
            &pThis->m_piProcInfo // PROCESS_INFORMATION 구조체
        ))
        {
            std::cerr << "프로세스 생성 실패\n";
            return 1;
        }

        // 파이프 쓰기 핸들 닫기
        if (!CloseHandle(pThis->m_hChildStdOutWrite))
        {
            return 1;
        }

        // 처음 입력줄을 skip할 경우: 처음 2줄은 무시한다.
        int skip_line = 0;

        for (;;)
        {
            dwRead = 0;
            // 자식 프로세스의 표준 출력에서 읽기
            bool done = !ReadFile(pThis->m_hChildStdOutRead, chBuf, 4096, &dwRead, NULL) || dwRead == 0;

            // 결과를 문자열에 추가
            if (dwRead > 0) {
                chBuf[dwRead] = '\0';
                strReadBuf += chBuf;
                strReadBuf = ConvertBinaryToString(strReadBuf);
                auto end_pos = std::remove(strReadBuf.begin(), strReadBuf.end(), '\n');
                strReadBuf.erase(end_pos, strReadBuf.end());

                if (strReadBuf.size() > 1) {
                    std::lock_guard<std::mutex> lock(pThis->m_mtx);
                    if(pThis->m_addText.size()>0)
                        pThis->m_addText += '\n';
                    pThis->m_addText += strReadBuf;
                    pThis->m_addFlag = true;
                }
            }
            else chBuf[0] = '\0';

            // 원문 영어 자막
            if (strReadBuf.size() > 1 && strReadBuf[0] == '-') {
                std::lock_guard<std::mutex> lock(pThis->m_mtx);
                if (pThis->m_addText.size() > 0)
                    pThis->m_strSrc += "\n";
                pThis->m_strSrc += strReadBuf.substr(1);
            }

            strReadBuf.clear();
            Sleep(5);
            if (pThis->m_isExit) break;
        }
    }

    return 0; // 스레드 종료
}

// 작업 Thread가 Sync로 읽어오기때문에 부수적으로 작업할 것들을 처리하기 위해 만듦: api 통신처리.
DWORD WINAPI BgJob::BgProc(LPVOID lpParam)
{
    BgJob* pThis = static_cast<BgJob*>(lpParam);

    while (1) {
        // Background Job 처리
        pThis->RunJobs();

        Sleep(100);
        if (pThis->m_isExit) break;
    }

    return 0;
}

// 번역 프로세스 종료
bool BgJob::TerminatePythonProcess() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "프로세스 스냅샷 생성 실패." << std::endl;
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        std::cerr << "첫 번째 프로세스 가져오기 실패." << std::endl;
        CloseHandle(hSnapshot);
        return false;
    }

    bool processTerminated = false;
    do {
        // 여기서는 "runTransWin.exe" 프로세스를 찾습니다.
        if (wcscmp(pe32.szExeFile, L"python.exe") == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                if (TerminateProcess(hProcess, 0)) {
                    std::cout << "python 프로세스가 성공적으로 종료되었습니다." << std::endl;
                    processTerminated = true;
                }
                else {
                    std::cerr << "파이썬 프로세스 종료 실패." << std::endl;
                }
                CloseHandle(hProcess);
            }
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return processTerminated;
}