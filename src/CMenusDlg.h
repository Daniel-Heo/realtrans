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
    //CRect rect; // �޴� �������� ��ġ�� ũ��
    //CImage normalImage; // �⺻ �̹���
    //CImage hoverImage; // ���콺 ���� �� �̹���
    Image *normalImage;
    Image *hoverImage;
    bool isHover; // ���콺 ���� ����

    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;

    CMenusDlg();
    ~CMenusDlg();

    /*
    BOOL LoadImages(const WCHAR* normalImagePath, const WCHAR* hoverImagePath);
    void Draw(HDC hdc);
    */
};

