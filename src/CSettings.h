#pragma once
#include <windows.h>
#include <string>

// 언어와 관련 데이터를 저장하는 구조체 정의
struct LanguageData {
    LPCWSTR language;
    LPCWSTR data;
};

class CSettings {
public:
    // 싱글톤 인스턴스 접근자
    static CSettings* GetInstance();

    // 인스턴스 소멸
    static void DestroyInstance();

    // 설정 초기화 및 기본값 설정
    void DefaultJson();

    // 설정 파일 읽기
    void ReadSettings(const std::string& filePath);

    // 설정 저장
    void SaveSettings(HWND hwnd, const std::string& filePath);
    void SaveSimpleSettings();

    // 설정 화면 갱신
    void RefreshSettings(HWND hwnd, BOOL isStart);

    // 설정 초기화
    void InitSettings(HWND hwnd);

    // Python 프로그램에 전달할 명령 생성
    void MakeChildCmd(const std::string& src_lang, const std::string& tgt_lang);

    // 설정 로딩
    void LoadSettings(HWND hwnd, const std::string& filePath);

    // 설정 정보 콘솔 출력
    void DumpSettings();

    // 폰트 사이즈 설정
    void SetFontSize(HWND hEdit);

    // 다이얼로그 프로시저 디스패처
    static INT_PTR CALLBACK StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 실제 다이얼로그 프로시저 구현
    INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 다이얼로그 생성 메서드
    BOOL Create(HWND hWndParent);

private:
    // 싱글톤 구현을 위한 private 생성자 및 소멸자
    CSettings();
    ~CSettings();

    // 싱글톤 인스턴스
    static CSettings* s_instance;

    // 다이얼로그 핸들
    HWND m_hDlg;

    // 인스턴스 핸들
    HINSTANCE m_hInst;

};

// 기존 다이얼로그 프로시저 선언 - 외부에서 접근하기 위한 전역 함수
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);