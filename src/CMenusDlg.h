#pragma once

//#include <windows.h>
//#include <vector>
//#include <afxwin.h> // MFC core and standard components
#include <atlimage.h>
#include <gdiplus.h>
//#include <gdiplusinit.h>
//
using namespace Gdiplus;
#pragma comment (lib,"gdiplus.lib")

class CMenusDlg
{
public:
    //CRect rect; // 메뉴 아이템의 위치와 크기
    //CImage normalImage; // 기본 이미지
    //CImage hoverImage; // 마우스 오버 시 이미지
    Image *normalImage;
    Image *hoverImage;
    bool isHover; // 마우스 오버 상태

    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;

    CMenusDlg();
    ~CMenusDlg();

    /*
    BOOL LoadImages(const WCHAR* normalImagePath, const WCHAR* hoverImagePath);
    void Draw(HDC hdc);
    */
};

