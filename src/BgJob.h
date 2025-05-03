#pragma once
#include <strsafe.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <mutex>
#include <tlhelp32.h>
#include "NemoViewer.h"

// 전방 선언
class CDlgSummary;
class CDlgTrans;

class BgJob {
public:
    BgJob();
    ~BgJob();

    // 싱글톤 접근자
    static BgJob* GetInstance();

    // 백그라운드 작업 초기화 및 시작
    bool Initialize();

    // 텍스트와 번역 처리를 위한 타이머 실행
    void RunTimer();

    // 리소스 정리 및 중지
    void Cleanup();

    // 외부 포인터 및 참조 설정
    void SetSummaryDialog(CDlgSummary* pDlgSum);
    void SetTransDialog(CDlgTrans* pDlgTrans);
    //void SetSettings(nlohmann::json& settingsRef);
    void SetViewer(NemoViewer* pViewer);

    // 뮤텍스 보호로 공유 데이터 접근
    bool AddTextToBuffer(const std::string& text);
    std::string GetSrcText();
    int GetSrcSize();

    // 요약 실행
    void SetRunSummary(bool value) { m_runSummary = value; }
    void ResetSummaryTime();

    // 스레드 제어 플래그
    void RequestExit();
    bool IsExitRequested() const;

    // 파이썬 프로세스 종료
    static bool TerminatePythonProcess();

    // 스레드 시작 (정적 메서드는 외부에서만 호출)
    bool StartThreads();

private:
    // 개인 메서드
    void RunJobs();

    // 스레드 프로시저
    static DWORD WINAPI ThreadProc(LPVOID lpParam);
    static DWORD WINAPI BgProc(LPVOID lpParam);
    static void CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime);

    // 스레드 핸들
    HANDLE m_hThread;
    HANDLE m_hBgThread;

    // 스레드 제어 변수
    bool m_bActThread;
    bool m_bBgThread;
    bool m_isExit;

    // 스레드 ID
    DWORD m_WorkThreadID;

    // 공유 데이터
    std::string m_addText;
    std::string m_strSrc;
    bool m_addFlag;

    // 요약 관련 변수
    time_t m_nSummaryTime;
    bool m_runSummary;

    // 외부 포인터
    CDlgSummary* m_pDlgSum;
    CDlgTrans* m_pDlgTrans;

    // 네모 에디터
    NemoViewer* m_pViewer; 

    // 뮤텍스
    mutable std::mutex m_mtx;

    // 파이프 핸들
    HANDLE m_hChildStdOutRead;
    HANDLE m_hChildStdOutWrite;

    // 프로세스 정보
    PROCESS_INFORMATION m_piProcInfo;

    // 클래스 인스턴스 (싱글톤 패턴용)
    static BgJob* s_instance;

};